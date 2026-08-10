#!/usr/bin/env python3
"""
mgba CLI debugger transport for Game Boy RPG development harness.
Uses mgba's command-line debugger (-d) via PTY with raw TTY mode.
Authoritative bridge for Game Boy snapshot, telemetry, and screen inspection.
"""
import subprocess, pty, os, select, time, tty, termios, fcntl, re

DEBUG_PROTOCOL_VERSION = 1

TELEMETRY_CAPACITY = 32
TELEMETRY_EVENT_SIZE = 13

GAME_STATE_MAP = {0: "OVERWORLD", 1: "BATTLE"}
MUSIC_TRACK_MAP = {0: "NONE", 1: "OVERWORLD", 2: "BATTLE"}
BATTLE_TURN_MAP = {0: "PLAYER", 1: "ENEMY_DELAY", 2: "ENEMY", 3: "RESULT"}
BATTLE_RESULT_MAP = {0: "NONE", 1: "VICTORY", 2: "DEFEAT"}
MAP_NAME_MAP = {0: "FIELD", 1: "TOWN"}
STORY_FLAG_ID_MAP = {1: "ARRIVED_TOWN", 2: "MET_MAYOR"}
ENTITY_ID_MAP = {0: "NONE", 1: "PLAYER", 2: "ENEMY", 3: "MAYOR", 4: "GUARD"}
DIALOGUE_ID_MAP = {0: "NONE", 1: "MAYOR_GREETING", 2: "GUARD_GREETING"}
EVENT_TYPE_MAP = {
    0: "PLAYER_MOVED", 1: "COLLISION", 2: "ENCOUNTER_STARTED",
    3: "BATTLE_STARTED", 4: "BATTLE_ACTION", 5: "DAMAGE_DEALT",
    6: "DAMAGE_RECEIVED", 7: "ENTITY_DEFEATED", 8: "BATTLE_WON",
    9: "BATTLE_LOST", 10: "GAME_STATE_CHANGED", 11: "MUSIC_CHANGED",
    12: "MAP_CHANGED", 13: "STORY_FLAG_SET", 14: "STORY_FLAG_CLEARED",
    15: "DIALOGUE_STARTED", 16: "DIALOGUE_NEXT", 17: "DIALOGUE_ENDED",
    18: "INTERACTION_ATTEMPT", 19: "RENDER_SCREEN", 20: "RENDER_DIALOGUE"
}
DIRECTION_MAP = {0: "UP", 1: "DOWN", 2: "LEFT", 3: "RIGHT"}

BUTTON_MASKS = {
    "RIGHT": 0x01, "LEFT": 0x02, "UP": 0x04, "DOWN": 0x08,
    "A": 0x10, "B": 0x20, "SELECT": 0x40, "START": 0x80
}

SCENARIO_IDS = {
    "NEW_GAME": 1, "FIRST_ENCOUNTER": 2, "TOWN_ARRIVAL": 3,
    "TOWN_DEPARTURE": 4, "TOWN_REENTRY": 5, "MAYOR_ENCOUNTER": 6,
    "MAYOR_DIALOGUE": 7, "MAYOR_DIALOGUE_MOVEMENT_BLOCKED": 8,
    "GUARD_DIALOGUE": 9, "FONT_TEST": 10, "DIALOGUE_RENDER_TEST": 11
}

def decode_story_flags(flags_mask):
    active = []
    for flag_id, name in STORY_FLAG_ID_MAP.items():
        if (flags_mask & (1 << (flag_id - 1))) != 0:
            active.append(name)
    return active

def load_sym_map(sym_path):
    syms = {}
    if os.path.exists(sym_path):
        with open(sym_path, 'r') as f:
            for line in f:
                parts = line.strip().split()
                if len(parts) == 2 and ':' in parts[0]:
                    addr_str = parts[0].split(':')[1]
                    name = parts[1]
                    try:
                        syms[name] = int(addr_str, 16)
                    except ValueError:
                        pass
    return syms


class TelemetryEventList(list):
    def __init__(self, events, events_lost=False, oldest_available_sequence=0):
        super().__init__(events)
        self.events_lost = events_lost
        self.oldest_available_sequence = oldest_available_sequence


class EmulatorSession:
    def __init__(self, rom_path="build/rpg_card_proto_debug.gb"):
        self.rom_path = rom_path
        self.sym_path = rom_path.replace(".gb", ".sym")
        self.master = None
        self.proc = None
        self.symbols = {}
        self.current_snapshot = {}

    def get_symbol(self, sym_name):
        for candidate in [sym_name, f"_{sym_name}"]:
            if candidate in self.symbols:
                return self.symbols[candidate]
        raise KeyError(f"HARNESS ERROR: Required symbol '{sym_name}' not found")

    # ── PTY I/O helpers ─────────────────────────────────────────────

    def _drain(self):
        while True:
            try:
                os.read(self.master, 4096)
            except (BlockingIOError, OSError):
                break

    def _send(self, cmd):
        self._drain()
        os.write(self.master, (cmd + '\n').encode())

    def _read_until(self, timeout=5.0, marker=b'> '):
        """Read PTY output until marker is found (returns full buffered data)."""
        buf = b''
        start = time.time()
        while time.time() - start < timeout:
            try:
                data = os.read(self.master, 4096)
                if data:
                    buf += data
                    if marker in buf:
                        return buf
            except (BlockingIOError, OSError):
                time.sleep(0.001)
        return buf

    def _cmd(self, cmd_str, timeout=3.0):
        """Send command and wait for prompt response."""
        self._send(cmd_str)
        return self._read_until(timeout)

    def _memread(self, addr):
        """Read single byte from GB memory address."""
        out = self._cmd(f'r/1 0x{addr:04X}').decode(errors='ignore')
        vals = re.findall(r'0x([0-9A-Fa-f]+)', out)
        return int(vals[-1], 16) if len(vals) >= 2 else None

    def _memwrite(self, addr, val):
        """Write single byte to GB memory address."""
        self._cmd(f'w/1 0x{addr:04X} 0x{val:02X}')

    def _set_pc(self, addr):
        """Set the CPU program counter."""
        self._cmd(f'w/r pc 0x{addr:04X}')

    def _step_until(self, target_pc, max_steps=500):
        """Step with 'next' until PC reaches target_pc."""
        for i in range(max_steps):
            out = self._cmd('next').decode(errors='ignore')
            m = re.search(r'PC:\s*([0-9A-F]+)', out)
            pc = int(m.group(1), 16) if m else 0
            if pc == target_pc:
                return pc
            if pc == 0:
                break
        return 0

    # ── Connection management ───────────────────────────────────────

    def connect(self):
        if not os.path.exists(self.rom_path):
            raise FileNotFoundError(f"ROM not found: {self.rom_path}")

        self.symbols = load_sym_map(self.sym_path)

        self.master, slave = pty.openpty()
        tty.setraw(self.master, termios.TCSANOW)
        flags = fcntl.fcntl(self.master, fcntl.F_GETFL)
        fcntl.fcntl(self.master, fcntl.F_SETFL, flags | os.O_NONBLOCK)

        cmd = ['xvfb-run', '--auto-servernum', 'mgba', '-d', self.rom_path]
        self.proc = subprocess.Popen(cmd, stdin=slave, stdout=slave, stderr=slave, close_fds=True)
        os.close(slave)

        time.sleep(2.0)
        self._drain()
        self._read_until(timeout=2.0)

        # Disable interrupts to prevent CRT0 interrupt storm
        self._memwrite(0xFF0F, 0x00)
        self._memwrite(0xFFFF, 0x00)

        # Enable harness mode (skips blocking init)
        self._memwrite(self.get_symbol("g_harness_mode"), 0x01)

        game_render_addr = self.get_symbol("game_render")

        # Jump to main, skipping CRT0 blocking code
        self._set_pc(0x0200)

        # Set frame-sync breakpoint
        self._cmd(f'break 0x{game_render_addr:04X}')

        # Continue: game_init runs, reaches game_render breakpoint
        self._send('c')
        out = self._read_until(timeout=20.0)
        if b'Hit breakpoint' not in out:
            raise RuntimeError("connect: game_render breakpoint not hit")

    def disconnect(self):
        if self.proc:
            try:
                self.proc.kill()
                self.proc.wait(timeout=1)
            except Exception:
                pass
            self.proc = None
        if self.master:
            try:
                os.close(self.master)
            except Exception:
                pass
            self.master = None

    # ── Frame control ───────────────────────────────────────────────

    def step(self, frames=1):
        """Advance N frames using breakpoint at _game_render."""
        for _ in range(frames):
            self._send('next')
            time.sleep(0.05)
            self._drain()
            self._send('c')
            out = self._read_until(timeout=10.0)
            if b'Hit breakpoint' not in out:
                raise RuntimeError("Frame step failed: breakpoint not hit")

    def wait(self, frames):
        self.step(frames)

    # ── Input injection ─────────────────────────────────────────────

    def press(self, button):
        btn_upper = button.upper()
        if btn_upper not in BUTTON_MASKS:
            raise ValueError(f"Unknown button '{button}'")
        mask = BUTTON_MASKS[btn_upper]
        addr = self.get_symbol("g_inp_mask")
        self._memwrite(addr, mask)
        time.sleep(0.05)
        self.step(1)

    # ── Scenario loading ────────────────────────────────────────────

    def load_scenario(self, scenario_id_str):
        if scenario_id_str not in SCENARIO_IDS:
            raise ValueError(f"Unknown scenario ID '{scenario_id_str}'")
        sid = SCENARIO_IDS[scenario_id_str]
        addr = self.get_symbol("g_scen_load")
        self._memwrite(addr, sid)
        time.sleep(0.05)
        self.step(2)

    # ── State inspection ────────────────────────────────────────────

    def snapshot(self):
        """Read 17-byte snapshot from g_snap_buf."""
        addr = self.get_symbol("g_snap_buf")
        snap_bytes = []
        for i in range(17):
            b = self._memread(addr + i)
            if b is not None:
                snap_bytes.append(b)
        
        if len(snap_bytes) >= 13:
            parsed = {
                "game_state": GAME_STATE_MAP.get(snap_bytes[0], f"UNKNOWN_{snap_bytes[0]}"),
                "player_x": snap_bytes[1],
                "player_y": snap_bytes[2],
                "player_hp": snap_bytes[3],
                "enemy_hp": snap_bytes[4],
                "enemy_active": snap_bytes[5],
                "music_track": MUSIC_TRACK_MAP.get(snap_bytes[6], f"UNKNOWN_{snap_bytes[6]}"),
                "battle_turn": BATTLE_TURN_MAP.get(snap_bytes[7], f"UNKNOWN_{snap_bytes[7]}"),
                "battle_result": BATTLE_RESULT_MAP.get(snap_bytes[8], f"UNKNOWN_{snap_bytes[8]}"),
                "battle_player_hp": snap_bytes[9],
                "battle_enemy_hp": snap_bytes[10],
                "map_id": MAP_NAME_MAP.get(snap_bytes[11], f"UNKNOWN_{snap_bytes[11]}"),
                "story_flags": snap_bytes[12],
                "story_flags_active": decode_story_flags(snap_bytes[12]),
                "dialogue_active": bool(snap_bytes[13]) if len(snap_bytes) >= 14 else False,
                "dialogue_line": snap_bytes[14] if len(snap_bytes) >= 15 else 0,
                "dialogue_id": snap_bytes[15] if len(snap_bytes) >= 16 else 0,
                "dialogue_id_name": DIALOGUE_ID_MAP.get(snap_bytes[15], f"UNKNOWN_{snap_bytes[15]}") if len(snap_bytes) >= 16 else "NONE",
                "player_facing": DIRECTION_MAP.get(snap_bytes[16], f"UNKNOWN_{snap_bytes[16]}") if len(snap_bytes) >= 17 else "UNKNOWN"
            }
            self.current_snapshot = parsed
            return parsed
        return self.current_snapshot

    def get_screen_buf(self):
        """Read 18×20 characters from g_ui_screen_buf."""
        addr = self.get_symbol("g_ui_screen_buf")
        rows = []
        for y in range(18):
            row_chars = []
            for x in range(20):
                b = self._memread(addr + y * 21 + x)
                if b is not None:
                    row_chars.append(chr(b) if 32 <= b <= 126 else ' ')
                else:
                    row_chars.append(' ')
            rows.append(''.join(row_chars))
        return '\n'.join(rows)

    def get_telemetry(self, since_seq=None):
        """Read telemetry ring buffer from ROM memory."""
        count_addr = self.get_symbol("g_telemetry_count")
        head_addr = self.get_symbol("g_telemetry_head")
        buf_addr = self.get_symbol("g_telemetry_buffer")

        count_lo = self._memread(count_addr) or 0
        count_hi = self._memread(count_addr + 1) or 0
        count = count_lo | (count_hi << 8)

        head = self._memread(head_addr) or 0

        num_slots = TELEMETRY_CAPACITY if count >= TELEMETRY_CAPACITY else count
        oldest_slot = head if count >= TELEMETRY_CAPACITY else 0

        all_events = []
        for i in range(count):
            slot = (oldest_slot + i) % TELEMETRY_CAPACITY
            off = slot * TELEMETRY_EVENT_SIZE
            b = []
            for j in range(TELEMETRY_EVENT_SIZE):
                v = self._memread(buf_addr + off + j)
                if v is not None:
                    b.append(v)
            if len(b) < TELEMETRY_EVENT_SIZE:
                break

            seq = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)
            frame = b[4] | (b[5] << 8) | (b[6] << 16) | (b[7] << 24)
            ev_type_id = b[8]
            ev_type_str = EVENT_TYPE_MAP.get(ev_type_id, f"UNKNOWN_{ev_type_id}")
            data = b[9:13]

            ev_obj = {
                "seq": seq, "frame": frame, "type": ev_type_str,
                "type_id": ev_type_id, "data": data
            }
            if ev_type_str in ("STORY_FLAG_SET", "STORY_FLAG_CLEARED"):
                ev_obj["flag_name"] = STORY_FLAG_ID_MAP.get(data[0], f"FLAG_{data[0]}")
            elif ev_type_str in ("DIALOGUE_STARTED", "DIALOGUE_NEXT", "DIALOGUE_ENDED"):
                ev_obj["dialogue_id_name"] = DIALOGUE_ID_MAP.get(data[0], f"UNKNOWN_{data[0]}")
            elif ev_type_str == "INTERACTION_ATTEMPT":
                ev_obj["target_entity"] = ENTITY_ID_MAP.get(data[3], f"UNKNOWN_{data[3]}")
            
            all_events.append(ev_obj)

        events_lost = False
        oldest_avail_seq = all_events[0]["seq"] if all_events else 0
        if since_seq is not None and oldest_avail_seq > 0:
            if since_seq < oldest_avail_seq - 1:
                events_lost = True

        filtered = [ev for ev in all_events if since_seq is None or ev["seq"] > since_seq]
        return TelemetryEventList(filtered, events_lost=events_lost, oldest_available_sequence=oldest_avail_seq)

#!/usr/bin/env python3
"""
mgba CLI debugger transport for Game Boy RPG development harness.
Uses mgba's command-line debugger (-d) via PTY with raw TTY mode.
Authoritative bridge for Game Boy snapshot, telemetry, and screen inspection.
"""
import subprocess, pty, os, select, time, tty, termios, fcntl, re, signal

DEBUG_PROTOCOL_VERSION = 1

TELEMETRY_CAPACITY = 32
TELEMETRY_EVENT_SIZE = 13

GAME_STATE_MAP = {0: "OVERWORLD", 1: "BATTLE", 2: "GAME_OVER", 3: "THANKS"}
SCREEN_MAP = {0: "OVERWORLD", 1: "DIALOGUE", 2: "BATTLE", 3: "GAME_OVER", 4: "THANKS"}
SCENE_MAP = {0: "FIELD", 1: "TOWN", 2: "FOREST", 3: "MOUNTAIN_PASS", 4: "CASTLE"}
MUSIC_TRACK_MAP = {0: "NONE", 1: "OVERWORLD", 2: "BATTLE"}
BATTLE_TURN_MAP = {0: "PLAYER", 1: "ENEMY_DELAY", 2: "ENEMY", 3: "RESULT"}
BATTLE_RESULT_MAP = {0: "NONE", 1: "VICTORY", 2: "DEFEAT"}
MAP_NAME_MAP = {0: "FIELD", 1: "TOWN", 2: "FOREST", 3: "MOUNTAIN_PASS", 4: "CASTLE"}
STORY_FLAG_ID_MAP = {1: "ARRIVED_TOWN", 2: "MET_MAYOR"}
ENTITY_ID_MAP = {0: "NONE", 1: "PLAYER", 2: "SLIME", 3: "MAYOR", 4: "GUARD",
                 5: "SHOPKEEPER", 6: "BAT"}
INTERACTION_ID_MAP = {0: "NONE", 1: "DIALOGUE", 2: "COMBAT"}
DIALOGUE_ID_MAP = {0: "NONE", 1: "MAYOR_GREETING", 2: "GUARD_GREETING",
                   3: "SHOPKEEPER_GREETING"}
BATTLE_ID_MAP = {0: "NONE", 1: "SLIME", 2: "BAT"}
EVENT_TYPE_MAP = {
    0: "PLAYER_MOVED", 1: "COLLISION", 2: "ENCOUNTER_STARTED",
    3: "BATTLE_STARTED", 4: "BATTLE_ACTION", 5: "DAMAGE_DEALT",
    6: "DAMAGE_RECEIVED", 7: "ENTITY_DEFEATED", 8: "BATTLE_WON",
    9: "BATTLE_LOST", 10: "GAME_STATE_CHANGED", 11: "MUSIC_CHANGED",
    12: "MAP_CHANGED", 13: "STORY_FLAG_SET", 14: "STORY_FLAG_CLEARED",
    15: "DIALOGUE_STARTED", 16: "DIALOGUE_NEXT", 17: "DIALOGUE_ENDED",
    18: "INTERACTION_ATTEMPT", 19: "RENDER_SCREEN", 20: "RENDER_DIALOGUE",
    21: "SCREEN_CHANGED", 22: "SCENE_CHANGED",
    23: "ACTOR_COLLISION", 24: "ACTOR_INTERACTION", 25: "ACTOR_COMBAT_START",
    26: "VARIABLE_SET", 27: "ITEM_ADDED", 28: "ITEM_REMOVED",
    29: "ACTOR_STATE_CHANGE"
}
DIRECTION_MAP = {0: "UP", 1: "DOWN", 2: "LEFT", 3: "RIGHT"}

# Static per-actor semantics resolved from the snapshot's actor ids.
ACTOR_INFO_MAP = {
    "PLAYER":      {"visual": "@", "hostile": False, "interaction": "NONE", "dialogue": "NONE", "battle": "NONE"},
    "SLIME":       {"visual": "E", "hostile": True,  "interaction": "COMBAT", "dialogue": "NONE", "battle": "SLIME"},
    "MAYOR":       {"visual": "M", "hostile": False, "interaction": "DIALOGUE", "dialogue": "MAYOR_GREETING", "battle": "NONE"},
    "GUARD":       {"visual": "G", "hostile": False, "interaction": "DIALOGUE", "dialogue": "GUARD_GREETING", "battle": "NONE"},
    "SHOPKEEPER":  {"visual": "S", "hostile": False, "interaction": "DIALOGUE", "dialogue": "SHOPKEEPER_GREETING", "battle": "NONE"},
    "BAT":         {"visual": "V", "hostile": True,  "interaction": "COMBAT", "dialogue": "NONE", "battle": "BAT"},
}

BUTTON_MASKS = {
    "RIGHT": 0x01, "LEFT": 0x02, "UP": 0x04, "DOWN": 0x08,
    "A": 0x10, "B": 0x20, "SELECT": 0x40, "START": 0x80
}

SCENARIO_IDS = {
    "NEW_GAME": 1, "FIRST_ENCOUNTER": 2, "TOWN_ARRIVAL": 3,
    "TOWN_DEPARTURE": 4, "TOWN_REENTRY": 5, "MAYOR_ENCOUNTER": 6,
    "MAYOR_DIALOGUE": 7, "MAYOR_DIALOGUE_MOVEMENT_BLOCKED": 8,
    "GUARD_DIALOGUE": 9, "FONT_TEST": 10, "DIALOGUE_RENDER_TEST": 11,
    "BATTLE_ATTACK": 12, "GUARD_INTERACTION_DISTANCE": 13, "GAME_OVER": 14,
    "OVERWORLD_BOOT": 15, "DIALOGUE_BOOT": 16, "BATTLE_BOOT": 17,
    "GAME_OVER_BOOT": 18, "THANKS_BOOT": 19, "FOREST_BOOT": 20,
    "MOUNTAIN_PASS_BOOT": 21, "CASTLE_BOOT": 22, "TOWN_BOOT": 23,
    "ACTOR_COLLISION_BLOCKING": 24, "ACTOR_SHOPKEEPER": 25, "ACTOR_BAT": 26
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
        self.proc = subprocess.Popen(cmd, stdin=slave, stdout=slave, stderr=slave,
                                     close_fds=True, start_new_session=True)
        os.close(slave)

        time.sleep(2.0)
        self._drain()
        self._read_until(timeout=2.0)

        game_render_addr = self.get_symbol("game_render")
        main_addr = self.get_symbol("main")

        # Enable harness mode before main() runs
        self._memwrite(self.get_symbol("g_harness_mode"), 0x01)

        # Jump to main (bypasses CRT0 blocking calls: display_off, DMA)
        self._set_pc(main_addr)

        # Verify we landed at main via breadcrumb (tolerate None if memory read fails)
        boot_phase_addr = self.get_symbol("g_boot_phase")
        boot_phase = self._memread(boot_phase_addr)

        # Set frame-sync breakpoint and run to first frame.
        # Retry: if the breakpoint fired but its output raced with a drain,
        # re-sync PC to main and try again.
        self._cmd(f'break 0x{game_render_addr:04X}')
        hit = False
        for _ in range(8):
            self._send('c')
            out = self._read_until(timeout=10.0)
            if b'Hit breakpoint' in out:
                hit = True
                break
            # Not hit — the emulator may have raced ahead or paused elsewhere.
            # Re-sync to main and retry.
            self._set_pc(main_addr)
            time.sleep(0.1)
        if not hit:
            raise RuntimeError("connect: game_render breakpoint not hit")

        # First breakpoint may fire inside game_init() inner game_render().
        # Advance one frame to reach the main-loop game_render().
        boot_phase_addr = self.get_symbol("g_boot_phase")
        boot_phase = self._memread(boot_phase_addr)
        if boot_phase is None or boot_phase < 2:
            raise RuntimeError(
                f"connect: g_boot_phase={boot_phase}, expected >= 2. "
                "main() or ui_init() may not have executed"
            )

        # Advance to next game_render (from main loop, after game_init returns)
        self._send('next')
        time.sleep(0.005)
        self._drain()
        self._send('c')
        out = self._read_until(timeout=10.0)

        boot_phase = self._memread(boot_phase_addr)
        if boot_phase != 4:
            raise RuntimeError(
                f"connect: g_boot_phase={boot_phase}, expected 4. "
                "Boot sequence incomplete: main→ui_init→game_init→first_render"
            )

    def disconnect(self):
        if self.proc:
            try:
                os.killpg(self.proc.pid, signal.SIGTERM)
            except Exception:
                pass
            try:
                self.proc.wait(timeout=1)
            except Exception:
                pass
            if self.proc.poll() is None:
                try:
                    os.killpg(self.proc.pid, signal.SIGKILL)
                except Exception:
                    pass
            try:
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
            time.sleep(0.005)
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
        time.sleep(0.005)
        self.step(1)

    # ── Scenario loading ────────────────────────────────────────────

    def load_scenario(self, scenario_id_str):
        if scenario_id_str not in SCENARIO_IDS:
            raise ValueError(f"Unknown scenario ID '{scenario_id_str}'")
        sid = SCENARIO_IDS[scenario_id_str]
        addr = self.get_symbol("g_scen_load")
        self._memwrite(addr, sid)
        time.sleep(0.005)
        self.step(2)

    # ── State inspection ────────────────────────────────────────────

    SNAPSHOT_BASE_SIZE = 20
    SNAPSHOT_ACTOR_ENTRY_SIZE = 4
    MAX_SNAPSHOT_ACTORS = 4
    SNAPSHOT_TOTAL_SIZE = SNAPSHOT_BASE_SIZE + (MAX_SNAPSHOT_ACTORS * SNAPSHOT_ACTOR_ENTRY_SIZE)

    def snapshot(self):
        """Read SNAPSHOT_TOTAL_SIZE bytes from g_snap_buf."""
        addr = self.get_symbol("g_snap_buf")
        snap_bytes = []
        for i in range(self.SNAPSHOT_TOTAL_SIZE):
            b = self._memread(addr + i)
            if b is not None:
                snap_bytes.append(b)
        
        if len(snap_bytes) >= 13:
            parsed = {
                "game_state": GAME_STATE_MAP.get(snap_bytes[0], f"UNKNOWN_{snap_bytes[0]}"),
                "screen": SCREEN_MAP.get(snap_bytes[18], f"UNKNOWN_{snap_bytes[18]}") if len(snap_bytes) >= 19 else "UNKNOWN",
                "screen_id": snap_bytes[18] if len(snap_bytes) >= 19 else None,
                "scene": SCENE_MAP.get(snap_bytes[19], f"UNKNOWN_{snap_bytes[19]}") if len(snap_bytes) >= 20 else "UNKNOWN",
                "scene_id": snap_bytes[19] if len(snap_bytes) >= 20 else None,
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
                "player_facing": DIRECTION_MAP.get(snap_bytes[16], f"UNKNOWN_{snap_bytes[16]}") if len(snap_bytes) >= 17 else "UNKNOWN",
                "game_over_choice": snap_bytes[17] if len(snap_bytes) >= 18 else 0,
                "actors": self._parse_actors(snap_bytes)
            }
            self.current_snapshot = parsed
            return parsed
        return self.current_snapshot

    def _parse_actors(self, snap_bytes):
        """Parse scene actors from bytes 20+ as (id, x, y, facing) entries."""
        actors = []
        if len(snap_bytes) < self.SNAPSHOT_BASE_SIZE:
            return actors
        entry = self.SNAPSHOT_ACTOR_ENTRY_SIZE
        base = self.SNAPSHOT_BASE_SIZE
        for i in range(self.MAX_SNAPSHOT_ACTORS):
            off = base + i * entry
            if off + entry > len(snap_bytes):
                break
            eid = snap_bytes[off]
            if eid == 0:
                continue
            id_name = ENTITY_ID_MAP.get(eid, f"UNKNOWN_{eid}")
            info = ACTOR_INFO_MAP.get(id_name, {})
            actors.append({
                "id": eid,
                "id_name": id_name,
                "x": snap_bytes[off + 1],
                "y": snap_bytes[off + 2],
                "facing": DIRECTION_MAP.get(snap_bytes[off + 3], f"UNKNOWN_{snap_bytes[off + 3]}"),
                "visual": info.get("visual", "?"),
                "hostile": info.get("hostile", False),
                "interaction": info.get("interaction", "NONE"),
                "dialogue": info.get("dialogue", "NONE"),
                "battle": info.get("battle", "NONE"),
            })
        return actors

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

    def get_screen_dump(self):
        """Read 20x18 VRAM background tilemap from 0x9800.
        
        Assumes GBDK font_ibm loaded at tile base 0 (identity tile-to-ASCII
        mapping).  For custom fonts the tile-base offset from the font handle
        would need to be subtracted first.
        """
        rows = []
        for y in range(18):
            # VRAM tilemap stride is 32 tiles (20 visible + 12 off-screen)
            row_bytes = []
            for x in range(20):
                b = self._memread(0x9800 + y * 32 + x)
                if b is not None:
                    row_bytes.append(chr(b) if 32 <= b <= 126 else '.')
                else:
                    row_bytes.append('.')
            rows.append(''.join(row_bytes))
        return '\n'.join(rows)

    def get_telemetry(self, since_seq=None):
        """Read telemetry ring buffer from ROM memory."""
        count_addr = self.get_symbol("g_telemetry_count")
        head_addr = self.get_symbol("g_telemetry_head")
        buf_addr = self.get_symbol("g_telemetry_buffer")

        count_lo = self._memread(count_addr)
        if count_lo is None:
            count_lo = 0
        count = count_lo

        head = self._memread(head_addr)
        if head is None:
            head = 0

        num_slots = TELEMETRY_CAPACITY if count >= TELEMETRY_CAPACITY else count
        oldest_slot = head if count >= TELEMETRY_CAPACITY else 0

        all_events = []
        for i in range(num_slots):
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
            elif ev_type_str == "ACTOR_COLLISION":
                ev_obj["actor"] = ENTITY_ID_MAP.get(data[2], f"UNKNOWN_{data[2]}")
            elif ev_type_str == "ACTOR_INTERACTION":
                ev_obj["actor"] = ENTITY_ID_MAP.get(data[2], f"UNKNOWN_{data[2]}")
                ev_obj["interaction"] = INTERACTION_ID_MAP.get(data[3], f"UNKNOWN_{data[3]}")
            elif ev_type_str == "ACTOR_COMBAT_START":
                ev_obj["actor"] = ENTITY_ID_MAP.get(data[0], f"UNKNOWN_{data[0]}")
            
            all_events.append(ev_obj)

        events_lost = False
        oldest_avail_seq = all_events[0]["seq"] if all_events else 0
        if since_seq is not None and oldest_avail_seq > 0:
            if since_seq < oldest_avail_seq - 1:
                events_lost = True

        filtered = [ev for ev in all_events if since_seq is None or ev["seq"] > since_seq]
        return TelemetryEventList(filtered, events_lost=events_lost, oldest_available_sequence=oldest_avail_seq)

#!/usr/bin/env python3
"""
Real SameBoy emulator session controller using interactive stdin/stdout PTY transport.
Loads dynamic symbol addresses from .sym file and executes memory inspection/writes via SameBoy debugger.
Authoritative bridge for Game Boy snapshot inspection and event telemetry retrieval.
"""

import subprocess
import pty
import os
import select
import time
import re

DEBUG_PROTOCOL_VERSION = 1

TELEMETRY_CAPACITY = 32
TELEMETRY_EVENT_SIZE = 13  # 4B seq (uint32) + 4B frame (uint32) + 1B type (uint8) + 4B data

GAME_STATE_MAP = {
    0: "OVERWORLD",
    1: "BATTLE"
}

MUSIC_TRACK_MAP = {
    0: "NONE",
    1: "OVERWORLD",
    2: "BATTLE"
}

BATTLE_TURN_MAP = {
    0: "PLAYER",
    1: "ENEMY_DELAY",
    2: "ENEMY",
    3: "RESULT"
}

BATTLE_RESULT_MAP = {
    0: "NONE",
    1: "VICTORY",
    2: "DEFEAT"
}

MAP_NAME_MAP = {
    0: "FIELD",
    1: "TOWN"
}

STORY_FLAG_ID_MAP = {
    1: "ARRIVED_TOWN",
    2: "MET_MAYOR"
}

ENTITY_ID_MAP = {
    0: "NONE",
    1: "PLAYER",
    2: "ENEMY",
    3: "MAYOR",
    4: "GUARD"
}

DIALOGUE_ID_MAP = {
    0: "NONE",
    1: "MAYOR_GREETING"
}

EVENT_TYPE_MAP = {
    0: "PLAYER_MOVED",
    1: "COLLISION",
    2: "ENCOUNTER_STARTED",
    3: "BATTLE_STARTED",
    4: "BATTLE_ACTION",
    5: "DAMAGE_DEALT",
    6: "DAMAGE_RECEIVED",
    7: "ENTITY_DEFEATED",
    8: "BATTLE_WON",
    9: "BATTLE_LOST",
    10: "GAME_STATE_CHANGED",
    11: "MUSIC_CHANGED",
    12: "MAP_CHANGED",
    13: "STORY_FLAG_SET",
    14: "STORY_FLAG_CLEARED",
    15: "DIALOGUE_STARTED",
    16: "DIALOGUE_NEXT",
    17: "DIALOGUE_ENDED",
    18: "INTERACTION_ATTEMPT",
    19: "RENDER_SCREEN",
    20: "RENDER_DIALOGUE"
}

DIRECTION_MAP = {
    0: "UP",
    1: "DOWN",
    2: "LEFT",
    3: "RIGHT"
}

BUTTON_MASKS = {
    "RIGHT":  0x01,
    "LEFT":   0x02,
    "UP":     0x04,
    "DOWN":   0x08,
    "A":      0x10,
    "B":      0x20,
    "SELECT": 0x40,
    "START":  0x80
}

SCENARIO_IDS = {
    "NEW_GAME": 1,
    "FIRST_ENCOUNTER": 2,
    "TOWN_ARRIVAL": 3,
    "TOWN_DEPARTURE": 4,
    "TOWN_REENTRY": 5,
    "MAYOR_ENCOUNTER": 6,
    "MAYOR_DIALOGUE": 7,
    "MAYOR_DIALOGUE_MOVEMENT_BLOCKED": 8,
    "GUARD_DIALOGUE": 9
}

def decode_story_flags(flags_mask):
    """Decodes uint32 story flags bitfield mask into active flag names."""
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
    """Custom list subclass attaching protocol lost-event metadata."""
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
        """Lookup a symbol address. Raises KeyError if not found in symbol map."""
        for candidate in [sym_name, f"_{sym_name}"]:
            if candidate in self.symbols:
                return self.symbols[candidate]
        raise KeyError(f"HARNESS ERROR: Required symbol '{sym_name}' not found in debug symbol map ({self.sym_path}). Ensure ROM is built with 'make debug'.")

    def connect(self):
        """Launch SameBoy in headless mode under PTY and wait until boot completes."""
        if not os.path.exists(self.rom_path):
            raise FileNotFoundError(f"ROM not found: {self.rom_path}. Run 'make debug' first.")

        self.symbols = load_sym_map(self.sym_path)

        self.master, slave = pty.openpty()
        cmd = ["xvfb-run", "--auto-servernum", "stdbuf", "-i0", "-o0", "-e0", "sameboy", "-s", self.rom_path]
        self.proc = subprocess.Popen(cmd, stdin=slave, stdout=slave, stderr=slave, close_fds=True)
        os.close(slave)

        # Wait for SameBoy CLI debugger prompt '>' before sending commands
        start_time = time.time()
        buf = ""
        while time.time() - start_time < 4.0:
            r, _, _ = select.select([self.master], [], [], 0.05)
            if r:
                try:
                    data = os.read(self.master, 4096).decode('utf-8', errors='ignore')
                    buf += data
                    if ">" in buf or "SameBoy" in buf:
                        break
                except OSError:
                    break

        time.sleep(0.1)
        self._drain()
        self._send_cmd("breakpoint _game_render")
        time.sleep(0.05)
        # Advance 5 frames so main() game_init() finishes boot initialization
        self.step(5)

    def disconnect(self):
        """Terminate SameBoy emulator process."""
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

    def _send_cmd(self, command):
        if self.master:
            os.write(self.master, (command + "\n").encode('utf-8'))
            time.sleep(0.02)

    def _drain(self):
        """Drain pending PTY output."""
        while True:
            r, _, _ = select.select([self.master], [], [], 0.005)
            if not r:
                break
            try:
                os.read(self.master, 4096)
            except OSError:
                break

    def _wait_for_breakpoint_hit(self, timeout=3.0):
        """Wait until SameBoy hits breakpoint at game_render."""
        start_time = time.time()
        buf = ""
        while time.time() - start_time < timeout:
            r, _, _ = select.select([self.master], [], [], 0.02)
            if r:
                try:
                    data = os.read(self.master, 4096)
                    if not data:
                        break
                    text = data.decode('utf-8', errors='ignore')
                    buf += text
                    clean_text = re.sub(r'\x1b\[[0-9;]*[mGKB]', '', buf)
                    if "Breakpoint 1: PC =" in clean_text or "PC = game_render" in clean_text or "PC = game_ren" in clean_text:
                        return buf
                except OSError:
                    break
        return buf

    def step(self, frames=1):
        """Step execution by N game frames (synchronizing on _game_render breakpoint)."""
        last_out = ""
        for _ in range(frames):
            self._drain()
            self._send_cmd("continue")
            last_out = self._wait_for_breakpoint_hit(timeout=3.0)
        return last_out

    def _write_mem_byte(self, addr, value):
        """Write a single byte to GB memory via eval [$addr] = val."""
        self._drain()
        self._send_cmd(f"eval [${addr:x}] = {value}")
        time.sleep(0.05)

    def _read_mem_bytes(self, addr, length):
        """Read `length` contiguous bytes starting at `addr` by querying 16-byte aligned blocks."""
        memory_map = {}
        start_base = addr & 0xFFF0
        end_base = (addr + length - 1) & 0xFFF0

        for curr_addr in range(start_base, end_base + 16, 16):
            self._drain()
            self._send_cmd(f"examine ${curr_addr:04x}")
            start_time = time.time()
            raw = ""
            block_received = False
            while time.time() - start_time < 0.4:
                r, _, _ = select.select([self.master], [], [], 0.02)
                if r:
                    try:
                        data = os.read(self.master, 4096).decode('utf-8', errors='ignore')
                        raw += data
                        clean = re.sub(r'\x1b\[[0-9;]*[mGKB]', '', raw)
                        lines = clean.split('\n')
                        for line in lines:
                            line_clean = line.strip().lower()
                            m = re.match(r'^(?:>\s*)?([0-9a-f]{4}):\s+([0-9a-f]{2}(?:\s+[0-9a-f]{2})+)', line_clean)
                            if m:
                                line_addr = int(m.group(1), 16)
                                bytes_in_line = [int(b, 16) for b in m.group(2).split()]
                                for idx, b_val in enumerate(bytes_in_line):
                                    memory_map[line_addr + idx] = b_val
                        if all((curr_addr + i) in memory_map for i in range(16)):
                            block_received = True
                            break
                    except OSError:
                        break
                if block_received:
                    break

        return [memory_map[addr + i] for i in range(length) if (addr + i) in memory_map]

    def load_scenario(self, scenario_id_str):
        """Write scenario ID to g_scen_load and advance 2 frames so ROM processes it."""
        if scenario_id_str not in SCENARIO_IDS:
            raise ValueError(f"HARNESS ERROR: Unknown scenario ID '{scenario_id_str}'. Registered scenarios: {list(SCENARIO_IDS.keys())}")
        scenario_num = SCENARIO_IDS[scenario_id_str]
        addr = self.get_symbol("g_scen_load")
        self._write_mem_byte(addr, scenario_num)
        time.sleep(0.05)
        self._drain()
        # Step 2 frames: frame 1 runs scenario_check_and_load, frame 2 captures snapshot
        self.step(2)

    def press(self, button):
        """Write button mask to g_inp_mask and advance 1 frame."""
        btn_upper = button.upper()
        if btn_upper not in BUTTON_MASKS:
            raise ValueError(f"HARNESS ERROR: Unknown button '{button}'. Valid buttons: {list(BUTTON_MASKS.keys())}")
        mask = BUTTON_MASKS[btn_upper]
        addr = self.get_symbol("g_inp_mask")
        self._write_mem_byte(addr, mask)
        time.sleep(0.05)
        self._drain()
        self.step(1)

    def wait(self, frames):
        """Wait/advance N frames."""
        self.step(frames)

    def snapshot(self):
        """Read full 17-byte snapshot from ROM memory at _g_snap_buf."""
        addr = self.get_symbol("g_snap_buf")
        snap_bytes = self._read_mem_bytes(addr, 17)
        if len(snap_bytes) >= 13:
            parsed = {
                "game_state":       GAME_STATE_MAP.get(snap_bytes[0], f"UNKNOWN_{snap_bytes[0]}"),
                "player_x":         snap_bytes[1],
                "player_y":         snap_bytes[2],
                "player_hp":        snap_bytes[3],
                "enemy_hp":         snap_bytes[4],
                "enemy_active":     snap_bytes[5],
                "music_track":      MUSIC_TRACK_MAP.get(snap_bytes[6], f"UNKNOWN_{snap_bytes[6]}"),
                "battle_turn":      BATTLE_TURN_MAP.get(snap_bytes[7], f"UNKNOWN_{snap_bytes[7]}"),
                "battle_result":    BATTLE_RESULT_MAP.get(snap_bytes[8], f"UNKNOWN_{snap_bytes[8]}"),
                "battle_player_hp": snap_bytes[9],
                "battle_enemy_hp":  snap_bytes[10],
                "map_id":              MAP_NAME_MAP.get(snap_bytes[11], f"UNKNOWN_{snap_bytes[11]}"),
                "story_flags":         snap_bytes[12],
                "story_flags_active":  decode_story_flags(snap_bytes[12]),
                "dialogue_active":     bool(snap_bytes[13]) if len(snap_bytes) >= 14 else False,
                "dialogue_line":       snap_bytes[14] if len(snap_bytes) >= 15 else 0,
                "dialogue_id":         snap_bytes[15] if len(snap_bytes) >= 16 else 0,
                "dialogue_id_name":    DIALOGUE_ID_MAP.get(snap_bytes[15], f"UNKNOWN_{snap_bytes[15]}") if len(snap_bytes) >= 16 else "NONE",
                "player_facing":       DIRECTION_MAP.get(snap_bytes[16], f"UNKNOWN_{snap_bytes[16]}") if len(snap_bytes) >= 17 else "UNKNOWN"
            }
            self.current_snapshot = parsed
            return parsed
        return self.current_snapshot

    def get_screen_dump(self):
        """Read 20x18 VRAM background tilemap from 0x9800 and return ASCII screen dump."""
        rows = []
        for y in range(18):
            row_bytes = self._read_mem_bytes(0x9800 + y * 32, 20)
            row_str = "".join(chr(b) if 32 <= b <= 126 else "." for b in row_bytes)
            rows.append(row_str)
        return "\n".join(rows)

    def get_telemetry(self, since_seq=None):
        """
        Read telemetry ring buffer from ROM memory in strict chronological order [oldest -> newest].
        Reads g_telemetry_count and g_telemetry_head.
        Decodes 13-byte GameEvent structures (uint32 seq, uint32 frame, uint8 type, 4x uint8 data).
        Optionally filters to events with seq > since_seq.
        Attaches events_lost and oldest_available_sequence metadata per protocol contract.
        """
        count_addr = self.get_symbol("g_telemetry_count")
        head_addr = self.get_symbol("g_telemetry_head")
        buf_addr = self.get_symbol("g_telemetry_buffer")

        count_bytes = self._read_mem_bytes(count_addr, 2)
        count = count_bytes[0] | (count_bytes[1] << 8) if len(count_bytes) >= 2 else 0

        head_bytes = self._read_mem_bytes(head_addr, 1)
        head = head_bytes[0] if len(head_bytes) >= 1 else 0

        # Read entire telemetry ring buffer from ROM RAM
        total_bytes_needed = (TELEMETRY_CAPACITY if count >= TELEMETRY_CAPACITY else count) * TELEMETRY_EVENT_SIZE
        raw_bytes = self._read_mem_bytes(buf_addr, total_bytes_needed)

        # Calculate physical slot index of oldest event in ring buffer
        oldest_slot = head if count >= TELEMETRY_CAPACITY else 0

        all_chronological_events = []
        for i in range(count):
            slot = (oldest_slot + i) % TELEMETRY_CAPACITY
            start = slot * TELEMETRY_EVENT_SIZE
            if start + TELEMETRY_EVENT_SIZE > len(raw_bytes):
                break
            b = raw_bytes[start:start + TELEMETRY_EVENT_SIZE]
            seq = b[0] | (b[1] << 8) | (b[2] << 16) | (b[3] << 24)
            frame = b[4] | (b[5] << 8) | (b[6] << 16) | (b[7] << 24)
            ev_type_id = b[8]
            ev_type_str = EVENT_TYPE_MAP.get(ev_type_id, f"UNKNOWN_{ev_type_id}")
            data = [b[9], b[10], b[11], b[12]]

            ev_obj = {
                "seq": seq,
                "frame": frame,
                "type": ev_type_str,
                "type_id": ev_type_id,
                "data": data
            }
            if ev_type_str in ("STORY_FLAG_SET", "STORY_FLAG_CLEARED"):
                flag_id = data[0]
                ev_obj["flag_id"] = flag_id
                ev_obj["flag_name"] = STORY_FLAG_ID_MAP.get(flag_id, f"FLAG_{flag_id}")
            elif ev_type_str in ("DIALOGUE_STARTED", "DIALOGUE_NEXT", "DIALOGUE_ENDED"):
                dialogue_id = data[0]
                ev_obj["dialogue_id"] = dialogue_id
                ev_obj["dialogue_id_name"] = DIALOGUE_ID_MAP.get(dialogue_id, f"UNKNOWN_{dialogue_id}")
            elif ev_type_str == "INTERACTION_ATTEMPT":
                ev_obj["target_x"] = data[0]
                ev_obj["target_y"] = data[1]
                ev_obj["facing"] = DIRECTION_MAP.get(data[2], f"UNKNOWN_{data[2]}")
                ev_obj["target_entity"] = ENTITY_ID_MAP.get(data[3], f"UNKNOWN_{data[3]}")

            all_chronological_events.append(ev_obj)

        oldest_avail_seq = all_chronological_events[0]["seq"] if all_chronological_events else 0
        events_lost = False
        if since_seq is not None and oldest_avail_seq > 0:
            if since_seq < oldest_avail_seq - 1:
                events_lost = True

        filtered_events = [
            ev for ev in all_chronological_events
            if (since_seq is None or ev["seq"] > since_seq)
        ]

        return TelemetryEventList(filtered_events, events_lost=events_lost, oldest_available_sequence=oldest_avail_seq)

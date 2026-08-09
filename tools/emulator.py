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
    11: "MUSIC_CHANGED"
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
    "FIRST_ENCOUNTER": 2
}

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
        """Read a block of contiguous bytes starting at exact target addr using SameBoy examine."""
        self._drain()
        self._send_cmd(f"examine ${addr:04x}")
        start_time = time.time()
        raw = ""
        memory_map = {}

        while time.time() - start_time < 0.6:
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
                    # Check if all requested bytes are present in memory_map
                    if all((addr + i) in memory_map for i in range(length)):
                        return [memory_map[addr + i] for i in range(length)]
                except OSError:
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
        """Read full 16-byte snapshot from ROM memory at _g_snap_buf."""
        addr = self.get_symbol("g_snap_buf")
        snap_bytes = self._read_mem_bytes(addr, 16)
        if len(snap_bytes) >= 11:
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
                "battle_enemy_hp":  snap_bytes[10]
            }
            self.current_snapshot = parsed
            return parsed
        return self.current_snapshot

    def get_telemetry(self):
        """
        Read telemetry ring buffer from ROM memory.
        Reads g_telemetry_count and decodes up to 32 GameEvent objects.
        Each GameEvent is 11 bytes:
          - seq: uint16 (bytes 0..1)
          - frame: uint32 (bytes 2..5)
          - type: uint8 (byte 6)
          - data: [byte 7, byte 8, byte 9, byte 10]
        """
        count_addr = self.get_symbol("g_telemetry_count")
        buf_addr = self.get_symbol("g_telemetry_buffer")

        count_bytes = self._read_mem_bytes(count_addr, 1)
        if not count_bytes:
            return []
        count = count_bytes[0]
        if count == 0:
            return []

        total_bytes_to_read = count * 11
        raw_bytes = self._read_mem_bytes(buf_addr, total_bytes_to_read)

        events = []
        for i in range(count):
            start = i * 11
            if start + 11 > len(raw_bytes):
                break
            b = raw_bytes[start:start + 11]
            seq = b[0] | (b[1] << 8)
            frame = b[2] | (b[3] << 8) | (b[4] << 16) | (b[5] << 24)
            ev_type_id = b[6]
            ev_type_str = EVENT_TYPE_MAP.get(ev_type_id, f"UNKNOWN_{ev_type_id}")
            data = [b[7], b[8], b[9], b[10]]
            events.append({
                "seq": seq,
                "frame": frame,
                "type": ev_type_str,
                "type_id": ev_type_id,
                "data": data
            })
        return events

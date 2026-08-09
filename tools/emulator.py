#!/usr/bin/env python3
"""
Real SameBoy emulator session controller using interactive stdin/stdout PTY transport.
Loads dynamic symbol addresses from .sym file and executes memory inspection/writes via SameBoy debugger.
"""

import subprocess
import pty
import os
import select
import time
import re

BUTTON_MASKS = {
    "RIGHT": 0x01,
    "LEFT":  0x02,
    "UP":    0x04,
    "DOWN":  0x08,
    "A":     0x10,
    "B":     0x20,
    "SELECT":0x40,
    "START": 0x80
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
        """Step execution by N game frames."""
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

    def load_scenario(self, scenario_id_str):
        """Write scenario ID to g_scen_load and advance 2 frames so ROM processes it."""
        scenario_num = SCENARIO_IDS.get(scenario_id_str, 1)
        addr = self.symbols.get("_g_scen_load", self.symbols.get("g_scen_load", 0xC2A4))
        self._write_mem_byte(addr, scenario_num)
        time.sleep(0.05)
        self._drain()
        # Step 2 frames: frame 1 runs scenario_check_and_load, frame 2 captures snapshot
        self.step(2)

    def press(self, button):
        """Write button mask to g_inp_mask and advance 1 frame."""
        mask = BUTTON_MASKS.get(button.upper(), 0)
        if mask:
            addr = self.symbols.get("_g_inp_mask", self.symbols.get("g_inp_mask", 0xC41A))
            self._write_mem_byte(addr, mask)
            time.sleep(0.05)
            self._drain()
            self.step(1)

    def wait(self, frames):
        """Wait/advance N frames."""
        self.step(frames)

    def snapshot(self):
        """Read snapshot from ROM memory at _g_snap_buf."""
        addr = self.symbols.get("_g_snap_buf", self.symbols.get("g_snap_buf", 0xC2A5))
        base_addr = addr & 0xFFF0
        offset = addr & 0x000F
        base_hex = f"{base_addr:04x}".lower()

        self._drain()
        self._send_cmd(f"examine ${addr:x}")

        start_time = time.time()
        raw = ""
        while time.time() - start_time < 0.5:
            r, _, _ = select.select([self.master], [], [], 0.02)
            if r:
                try:
                    data = os.read(self.master, 4096).decode('utf-8', errors='ignore')
                    raw += data
                    clean = re.sub(r'\x1b\[[0-9;]*[mGKB]', '', raw)
                    for line in reversed(clean.split('\n')):
                        line_clean = line.strip().lower()
                        m = re.match(r'^(?:>\s*)?([0-9a-f]{4}):\s+([0-9a-f]{2}(?:\s+[0-9a-f]{2})+)', line_clean)
                        if m:
                            line_addr = int(m.group(1), 16)
                            if line_addr == base_addr or line_addr == addr:
                                hex_bytes = [int(b, 16) for b in m.group(2).split()]
                                start_idx = offset if line_addr == base_addr else 0
                                snap_bytes = hex_bytes[start_idx:]
                                if len(snap_bytes) >= 7:
                                    parsed = {
                                        "game_state": "BATTLE" if snap_bytes[0] == 1 else "OVERWORLD",
                                        "player_x": snap_bytes[1],
                                        "player_y": snap_bytes[2],
                                        "player_hp": snap_bytes[3],
                                        "enemy_hp": snap_bytes[4],
                                        "enemy_active": snap_bytes[5],
                                        "music_track": {0: "NONE", 1: "OVERWORLD", 2: "BATTLE"}.get(snap_bytes[6], "OVERWORLD")
                                    }
                                    self.current_snapshot = parsed
                                    return parsed
                except OSError:
                    break
        return self.current_snapshot

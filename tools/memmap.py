#!/usr/bin/env python3
"""Print a reproducible memory budget for the debug ROM from its linker map.

Usage:
    python3 tools/memmap.py build/rpg_card_proto_debug.map

Reports the fixed code area (_CODE), the non-bankable _HOME area (which must
stay below 0x8000 on MBC5 -- CPU addresses >= 0x8000 alias VRAM), and the WRAM
_DATA area.  Exits non-zero if any documented invariant is violated.
"""
import re
import sys


# The RAM-resident VBlank ISR lives in WRAM at 0xC900-0xC93F (see
# src/crt0.s).  The Makefile pins _DATA at 0xC940 so no C data overlaps it;
# this range must stay free of every linker area.
ISR_RESERVE_START = 0xC900
ISR_RESERVE_END = 0xC940


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(1)

    areas = {}
    with open(sys.argv[1]) as f:
        for line in f:
            m = re.match(r"\s*(\w+)\s+([0-9A-Fa-f]+)\s+([0-9A-Fa-f]+)\s+=\s+(\d+)\.\s+bytes", line)
            if not m:
                continue
            name, start, size, nbytes = m.groups()
            areas[name] = (int(start, 16), int(size, 16), int(nbytes))

    ok = True

    print("Game Boy ROM memory budget")
    print("--------------------------")

    if "_CODE" in areas:
        start, size, nbytes = areas["_CODE"]
        print(f"_CODE (fixed bank code/data) : {nbytes:>6} B  @ 0x{start:04X}-0x{start + size:04X}")
    else:
        print("_CODE : (not found)")
        ok = False

    if "_HOME" in areas:
        start, size, nbytes = areas["_HOME"]
        end = start + size
        headroom = 0x8000 - end
        status = "OK" if end <= 0x8000 else "VIOLATION (>= 0x8000 aliases VRAM)"
        if end > 0x8000:
            ok = False
        print(f"_HOME (non-bankable)          : {nbytes:>6} B  @ 0x{start:04X}-0x{end:04X}  "
              f"headroom to 0x8000: {headroom} B  [{status}]")
    else:
        print("_HOME : (not found)")
        ok = False

    if "_DATA" in areas:
        start, size, nbytes = areas["_DATA"]
        end = start + size
        ram_headroom = 0xE000 - end
        print(f"_DATA (WRAM)                  : {nbytes:>6} B  @ 0x{start:04X}-0x{end:04X}  "
              f"headroom to 0xE000: {ram_headroom} B")
        if start < ISR_RESERVE_START:
            print(f"  VIOLATION: _DATA base 0x{start:04X} < ISR reserve 0x{ISR_RESERVE_START:04X} "
                  f"(Makefile must link with -Wl-b_DATA=0x{ISR_RESERVE_END:04X})")
            ok = False
    else:
        print("_DATA : (not found)")
        ok = False

    for name, (start, size, nbytes) in sorted(areas.items()):
        if name in ("_CODE", "_HOME", "_DATA"):
            continue
        end = start + size
        if start < ISR_RESERVE_END and end > ISR_RESERVE_START:
            print(f"{name} overlaps the reserved VBlank ISR range "
                  f"0x{ISR_RESERVE_START:04X}-0x{ISR_RESERVE_END:04X}: "
                  f"0x{start:04X}-0x{end:04X}")
            ok = False

    print()
    if ok:
        print("memory budget: invariants OK")
    else:
        print("memory budget: INVARIANT VIOLATION")
        sys.exit(1)


if __name__ == "__main__":
    main()

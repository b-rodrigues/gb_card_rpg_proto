# Foundation Baseline (pre-final freeze)

Known-good reference point captured before the Foundation 1.0 hardening
milestone.  Tag: `rpg-foundation-pre-final`.

## Baseline record

| Item | Value |
|---|---|
| Git commit | `1bf1f43` (fix: deep-review findings) |
| Harness scenarios | 89 (89/89 passing) |
| Release ROM size | 131072 bytes (128 KB, MBC5, 8 banks) |
| Debug ROM size | 131072 bytes |
| `_CODE` (bank-0 code/data) | 29356 B |
| `_HOME` (non-bankable) | 1863 B @ 0x74AC-0x7BF3 (headroom to 0x8000: 1037 B) |
| `_DATA` (WRAM) | 1224 B @ 0xC0A0-0xC568 |
| Save/load | host-side roundtrip only; no SRAM battery save yet |
| Graphics | ASCII console-font rendering; no tilesets/sprites/palettes |
| Emulators verified | mGBA (harness), SameBoy (`make run`); no Gambatte, no hardware |

## Known limitations at this point

* No SRAM battery save (in-session roundtrip only).
* ASCII rendering (graphics pipeline not yet built).
* Enemy battle stats hardcoded (`enemy.attack = 2`); no damage variance.
* Quest system: linear 0/1/2 status only; see docs/roadmap.md known gaps.
* No Gambatte/hardware verification available in the build environment.

## What this tag is for

A rollback point if the Foundation 1.0 work (SRAM save, header changes,
graphics pipeline, docs) goes sideways.

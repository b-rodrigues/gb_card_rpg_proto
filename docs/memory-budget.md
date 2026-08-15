# Memory Budget

Reproducible numbers from `make memmap` (parses the debug linker map and fails
on a `_HOME` >= 0x8000 violation).

## Current budget

```text
Game Boy ROM memory budget
--------------------------
_CODE (fixed bank code/data) : 29486 B  @ 0x0200-0x752E
_HOME (non-bankable)        :  1863 B  @ 0x752E-0x7C75  headroom to 0x8000:  907 B  [OK]
_DATA (WRAM)                :  1224 B  @ 0xC0A0-0xC568  headroom to 0xE000: 6808 B
```

## Where the bytes go

### ROM (128 KB MBC5, 8 banks)

* **`_CODE`** (~29.5 KB): all game + engine code and the `static const`
  content tables (event table, dialogue table, actor definitions, item
  catalog, quest registry, shop stock).  `_CODE` grows with every feature and
  with content tables.
* **`_HOME`** (1863 B): boot-critical code that must stay below `0x8000` —
  the timer ISR trampoline path (`_audio_update`), `_joypad`, `_set_bkg_data`
  and other non-bankable library routines.  This is the tightest budget:
  only 907 B of headroom.  Keep it there; do not grow it casually.
* Banked ROM (banks 1-7): currently unused by gameplay code (everything links
  into bank 0).  Banked data/code is available if `_CODE` outgrows bank 0.

### WRAM (8 KB, `0xC000-0xDFFF`)

* `_DATA` (1224 B): GBDK globals + the fixed-size runtime structs.  The big
  fixed consumers (approximate):
  * `GameState` ~ 200 B (party 13, inventory 33, variables 32, world 49,
    progression 49, flags 8, currency 8, scene 4, equipment 1)
  * `World` ~ 300 B (20x12 tile map = 240 B + player + 4 actor slots)
  * `Game` aggregates the above plus `Battle`, `DialogueState`,
    `RenderCache`, telemetry buffers.
* The remaining WRAM (0xC568-0xDFFF, ~6.8 KB) is the stack + any future
  graphics RAM needs.

### HRAM / VRAM / stack

* HRAM: only the timer ISR uses it transiently.
* VRAM: currently holds the console font (ASCII prototype).  The graphics
  pipeline (see `docs/graphics.md`) will consume the 8 KB BG tilemap +
  tilesets + OAM (40 sprites) budgets.
* Stack: GBDK default; fits comfortably in the free WRAM.

## Rules

* Run `make memmap` after any substantial feature; it exits non-zero on a
  `_HOME` violation.
* Keep `_CODE` (bank 0) as small as practical — prefer banked content when it
  grows.
* Every substantial feature should note its memory impact in
  `docs/roadmap.md`.

## Largest consumers to watch

1. Content tables (events, dialogue, actors) — all `static const` in bank 0.
2. The 20x12 world tile map in `World` (240 B of WRAM, will move to VRAM
   when tilesets replace ASCII).
3. `GameState` — the save unit; its size defines the SRAM save size.

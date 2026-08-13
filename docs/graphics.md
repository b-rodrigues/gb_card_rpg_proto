# Graphics (pipeline, first slice implemented)

The current prototype renders with the console font (ASCII); the player is a
real OAM sprite (see the `sprites` branch work).  This document specifies the
planned graphics pipeline.

## Goal

Replace the ASCII presentation with real Game Boy tiles/sprites while keeping
the game **mechanically identical**: same `GameState`, scenes, actors, quests,
battle, inventory, equipment, boss — different presentation.

## Pipeline

```text
PNG source assets
      ↓  tools/png2gb.py
validation (dimensions, 8x8 tile alignment, palette limits, sprite size,
            unsupported colors, duplicate tiles, tile counts)
      ↓
GB-native data (tileset bytes, tilemaps, OAM sprite defs, palettes)
      ↓
ROM (banked const tables)
```

`tools/png2gb.py` must produce actionable errors (which asset, which rule
violated) rather than silently emitting broken graphics.

## Status

Implemented:

* `tools/png2gb.py` — PNG → GB 2bpp tileset (tile-alignment, palette-limit,
  unsupported-color validation; exact canonical-shade matching, no lossy
  snapping).  Single-image → tileset only; tilemap/OAM/dedup are TODO.
* `make gfx` — regenerates `src/gfx/*.h` from `assets/*.png`
  (deterministic output; a CI step fails if the committed headers drift from
  the source assets).
* `assets/player_demo.png` → `src/gfx/player_sprite_tile.h`, included by
  `src/ui/ui.c` (byte-identical to the previously hand-authored tile).

TODO: tilemaps, OAM sprite definitions, duplicate-tile dedup, and the
renderer rewiring below.

## Renderer requirements (DMG + CGB)

* Tilemap background rendering via `set_bkg_data`/`set_bkg_tiles` replacing
  the console (`putchar`) path in `src/ui/ui.c`.
* OAM sprite engine for the player, NPCs, and enemies (40 sprite limit).
* CGB palettes (BG + OBJ) with a DMG grayscale fallback; reset `VBK_REG = 0`
  after attribute writes (AGENTS.md §38).
* Harness-safe: no VBlank waits in harness paths (`g_harness_mode` skips
  vsync; `ui_init` turns the LCD off before `font_load`) — AGENTS.md §52.6.
* Keep incremental redraws for player movement / HUD (AGENTS.md §36).

### Sprite transition-hide timing rule

A full-screen redraw (clear + redraw, ~360 per-char `putchar` writes) takes
several display sweeps — the screen visibly blanks and redraws top-to-bottom.
Any sprite state change around a transition must therefore be written to
**real OAM before the redraw starts**, not at the end of the frame:

* `game_render()` calls `ui_sprite_begin_transition()` (forces real OAM Y=0,
  preserves shadow position) before `screen_render()` whenever a full redraw
  is about to run (`render_cache` invalid or screen changed).
* `ui_sprite_commit()` (shadow OAM → real OAM DMA) runs once per frame in
  `main.c` **after `vsync()`**, i.e. during VBlank, so each displayed frame
  shows exactly the intended sprite state and the transition hide never
  reveals the sprite at a stale position over the wipe.

## Screens to rewire

overworld (tileset + animated player + NPC/enemy sprites), battle (enemy
sprite + text box), dialogue (text box), item/shop menus (menu frame),
ending/game-over.

## Stress test

Large varied tilemaps, many actors, simultaneous animation, DMG+CGB palette
switching, and the full transition chain
`overworld → town → dialogue → battle → victory → overworld`.  Measure ROM
growth, VRAM/WRAM, sprite/tile limits, and CPU timing — before the real game
depends on them.

## Harness

Semantic state stays authoritative; screenshots then show real pixels.  Add a
`GRAPHICS:` block (tileset, sprite set) to the semantic dump so an LLM never
needs to infer state from graphics.

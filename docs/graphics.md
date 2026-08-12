# Graphics (deferred milestone — spec)

The current prototype renders with the console font (ASCII).  This document
specifies the planned graphics pipeline so the work is pre-scoped.  It is a
design spec, not yet implemented.

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

## Renderer requirements (DMG + CGB)

* Tilemap background rendering via `set_bkg_data`/`set_bkg_tiles` replacing
  the console (`putchar`) path in `src/ui/ui.c`.
* OAM sprite engine for the player, NPCs, and enemies (40 sprite limit).
* CGB palettes (BG + OBJ) with a DMG grayscale fallback; reset `VBK_REG = 0`
  after attribute writes (AGENTS.md §38).
* Harness-safe: no VBlank waits in harness paths (`g_harness_mode` skips
  vsync; `ui_init` turns the LCD off before `font_load`) — AGENTS.md §52.6.
* Keep incremental redraws for player movement / HUD (AGENTS.md §36).

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

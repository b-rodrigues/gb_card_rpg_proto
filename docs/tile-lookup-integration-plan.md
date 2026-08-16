# Tile Lookup Integration Plan

## Purpose

Integrate `assets/RPG_interior.png` and `assets/RPG_exterior.png` as the first real world tilesets while preserving the current ASCII-driven map semantics. The renderer should translate existing scene glyphs, such as `#` for walls and `.` for floor/ground, through a lookup table into Game Boy background tile IDs.

This is a presentation-only migration. Collision, encounters, scripted triggers, stable entity IDs, and semantic debug output must continue to use the existing world and scene state as the source of truth.

## Goals

* Keep existing map authoring readable during the transition by retaining semantic glyph maps.
* Introduce a deterministic glyph-to-tile lookup table for interior and exterior maps.
* Render world backgrounds from generated tile data instead of console font characters.
* Keep collision and debug inspection independent from visual tile IDs.
* Support both DMG and CGB rendering, with explicit CGB palettes and safe VRAM bank handling.

## Non-goals

* Do not redesign the world format around visual tile IDs yet.
* Do not make gameplay collision depend on rendered tile numbers.
* Do not add a dynamic scripting language or runtime asset loader.
* Do not require screenshots for automated correctness checks; screenshots remain visual review aids only.

## Source Assets

| Asset | Intended use | Notes |
| --- | --- | --- |
| `assets/RPG_exterior.png` | Field, town exterior, roads, grass, water, fences, trees, cliffs, gates | Used by outdoor scenes. |
| `assets/RPG_interior.png` | Houses, shops, rooms, indoor floors, walls, counters, decorations | Used by indoor scenes and menu-like diegetic interiors. |

Before implementation, inspect each PNG's dimensions, tile grid, color count, and palette groupings. The conversion tool must reject assets that cannot be represented as Game Boy 2bpp tiles without an explicit palette strategy.

## Proposed Data Flow

```text
assets/RPG_exterior.png
assets/RPG_interior.png
        ↓
tools/png2gb.py or a focused tileset generation command
        ↓
src/gfx/rpg_exterior_tiles.h
src/gfx/rpg_interior_tiles.h
src/gfx/rpg_tile_lookup.h
        ↓
world scene glyph map + scene tileset kind
        ↓
glyph lookup table maps '#', '.', '+', '~', etc. to tile IDs
        ↓
set_bkg_data / set_bkg_tiles render real Game Boy tiles
```

The generated tile bytes should be deterministic and checked in, matching the existing `make gfx` workflow.

## Scene Tileset Selection

Add an explicit tileset kind to scene metadata rather than inferring it from scene names.

```c
typedef enum {
    WORLD_TILESET_EXTERIOR,
    WORLD_TILESET_INTERIOR
} WorldTilesetKind;
```

Each scene definition should declare which lookup table it uses:

```c
{
    .scene_id = SCENE_TOWN,
    .tileset = WORLD_TILESET_EXTERIOR,
    ...
}

{
    .scene_id = SCENE_SHOP,
    .tileset = WORLD_TILESET_INTERIOR,
    ...
}
```

If the current scene struct cannot accept this field cleanly, introduce a small companion table keyed by `SceneId` as an interim step. The long-term target is for scene render metadata to live with scene content, not in renderer-specific conditionals.

## Glyph Lookup Table

The lookup table maps semantic scene glyphs to visual tile entries. It should be small, explicit, and stable.

Example initial mapping:

| Glyph | Semantic meaning | Exterior tile | Interior tile | Collision |
| --- | --- | --- | --- | --- |
| `#` | Wall / solid boundary | stone wall, tree line, or cliff edge | room wall | solid |
| `.` | Walkable ground | grass or path | floor | walkable |
| `+` | Door / gate | town gate or doorway | interior door | trigger or walkable by scene rule |
| `~` | Water | water | unused or decorative water | solid unless scene says otherwise |
| `=` | Road / path | dirt path | rug or plank path | walkable |
| `C` | Counter / table | stall counter | shop counter | solid |
| `D` | Decoration | sign, bush, pot | pot, shelf, table | scene-specific |
| ` ` | Empty / filler | blank ground | blank floor | follow existing scene rule |

The exact tile IDs must be filled in after inspecting the tile coordinates in `RPG_exterior.png` and `RPG_interior.png`. Prefer symbolic names over numeric IDs in hand-written code.

```c
typedef struct {
    char glyph;
    uint8_t tile_id;
    uint8_t attr;
} TileGlyphEntry;

static const TileGlyphEntry exterior_glyph_tiles[] = {
    { '#', EXTERIOR_TILE_WALL, 0 },
    { '.', EXTERIOR_TILE_GRASS, 0 },
    { '+', EXTERIOR_TILE_DOOR, 0 },
    { '~', EXTERIOR_TILE_WATER, 0 }
};

static const TileGlyphEntry interior_glyph_tiles[] = {
    { '#', INTERIOR_TILE_WALL, 0 },
    { '.', INTERIOR_TILE_FLOOR, 0 },
    { '+', INTERIOR_TILE_DOOR, 0 },
    { 'C', INTERIOR_TILE_COUNTER, 0 }
};
```

The renderer should provide a safe fallback tile for unknown glyphs and emit debug-visible diagnostics in debug builds, rather than silently drawing random tile IDs.

## Tile Coordinate Catalog

Create a hand-authored catalog once the source PNGs are inspected:

```text
RPG_exterior.png
  tile (x=0, y=0): EXTERIOR_TILE_GRASS
  tile (x=1, y=0): EXTERIOR_TILE_PATH
  tile (x=2, y=0): EXTERIOR_TILE_WALL

RPG_interior.png
  tile (x=0, y=0): INTERIOR_TILE_FLOOR
  tile (x=1, y=0): INTERIOR_TILE_WALL
  tile (x=2, y=0): INTERIOR_TILE_COUNTER
```

Keep this catalog near the generated graphics headers or in a small checked-in metadata file. Do not scatter tile coordinate assumptions throughout renderer code.

## Implementation Phases

### Phase 1: Asset audit and conversion

1. Validate `RPG_interior.png` and `RPG_exterior.png` dimensions and ensure they align to 8x8 tiles.
2. Extend `tools/png2gb.py` or add a focused generation mode for these tilesets.
3. Generate deterministic C headers for both source PNGs.
4. Add `make gfx` drift checking for the new generated headers.

### Phase 2: Tile catalog and symbolic IDs

1. Create symbolic tile IDs for the first useful subset of exterior and interior tiles.
2. Document the PNG tile coordinates for each symbolic ID.
3. Avoid importing the entire asset sheet into gameplay-facing headers if only a subset is used.

### Phase 3: Scene render metadata

1. Add a scene-level `WorldTilesetKind` or companion table.
2. Keep game content in the game layer and renderer behavior in UI/world rendering code.
3. Ensure the debug snapshot can expose the active tileset kind in a concise `GRAPHICS:` block.

### Phase 4: Glyph-to-tile lookup renderer

1. Add lookup tables for exterior and interior glyphs.
2. Replace overworld console glyph drawing with background tile rendering.
3. Keep the player, NPCs, and enemies as sprites or temporary overlay glyphs until the sprite pass is ready.
4. Preserve targeted redraw behavior for movement; do not full-clear the screen on every step.

### Phase 5: Collision and semantic validation

1. Confirm collision still reads semantic scene data, not tile IDs.
2. Add or update a scenario for wall collision where `#` renders as a wall tile but collision behavior remains unchanged.
3. Add or update a scenario for a walkable floor/ground tile where `.` renders through the lookup table.
4. Ensure `SNAPSHOT` or formatted harness output reports the same player map and coordinates before and after the renderer migration.

### Phase 6: Visual review

1. Run the semantic harness first.
2. Run the release validation.
3. Capture screenshots for visual review only after semantic checks pass.
4. Compare field, town, and at least one interior scene to ensure the expected lookup table is selected.

## Testing Strategy

Required checks after implementation:

```bash
make gfx
make test-harness
make test
make screenshots
```

Add focused scenario coverage where behavior could accidentally couple to visual tiles:

* `movement_wall_collision`: player attempts to move into a `#` wall and remains in place.
* `movement_floor_walkable`: player moves across `.` floor/ground and emits `PLAYER_MOVED`.
* `tileset_exterior_scene`: snapshot reports the exterior tileset for an outdoor scene.
* `tileset_interior_scene`: snapshot reports the interior tileset for an indoor scene.

## Debug and Observability Requirements

The renderer migration should add a concise graphics section to the semantic dump:

```text
GRAPHICS:
  tileset: EXTERIOR
  lookup: exterior_glyph_tiles
  unknown_glyphs: 0
```

If a scene contains an unmapped glyph in debug builds, the diagnostic should include:

* scene ID;
* glyph;
* map coordinate;
* active tileset kind;
* fallback tile used.

This keeps failures machine-readable and avoids relying on a human spotting a bad tile in a screenshot.

## Game Boy Constraints

* Keep generated tile data in ROM and bank it deliberately.
* Keep renderer hot paths small and avoid per-frame lookup work when a cached tilemap can be reused.
* Respect VRAM access rules when loading tile data and tilemaps.
* For CGB attributes, write attributes through VRAM bank 1 and always restore `VBK_REG = 0` afterward.
* Initialize explicit CGB BG palettes for both tilesets and keep a DMG fallback.
* Avoid dynamic allocation and large temporary JSON/string buffers in ROM code.

## Open Questions

* Which exact tile coordinates in each PNG should be the canonical first-pass tiles for `#`, `.`, `+`, `~`, `=`, `C`, and decorations?
* Should animated tiles such as water be represented in the initial lookup table or deferred until after static rendering works?
* Are interior and exterior palettes small enough to fit the first-pass CGB palette plan, or do scenes need palette subsets?
* Should unknown glyphs fail debug builds immediately, or render a fallback tile while failing only a dedicated validation command?

## Acceptance Criteria

The first integration slice is complete when:

1. `assets/RPG_interior.png` and `assets/RPG_exterior.png` are converted into deterministic generated headers.
2. At least one exterior scene and one interior scene render with real background tiles through glyph lookup tables.
3. `#` maps to an appropriate wall tile and remains semantically solid.
4. `.` maps to an appropriate floor or ground tile and remains walkable.
5. The debug harness can report the active tileset and catch unmapped glyphs.
6. Existing gameplay scenarios still pass without screenshot-based assertions.

## First Draft Lookup Table From Current Code

This draft is based on the current renderer and scene model, not on a final visual audit of the PNG tile coordinates.

Current terrain is not stored as arbitrary ASCII per scene. `World.map` stores a `TileType`, and the renderer converts those four tile types through `g_sem_map` as:

```c
TILE_FLOOR    -> '.'
TILE_WALL     -> '#'
TILE_EXIT     -> '>'
TILE_BUILDING -> 'B'
```

Scene exits also carry a directional `tile_char` (`>` or `<`), but the current background renderer uses the generic `TILE_EXIT -> '>'` glyph for every exit. The first implementation can preserve that behavior, then optionally render directional exit variants once the tile lookup supports exit metadata.

The first-pass lookup should therefore cover the existing terrain glyphs first, then actor overlay glyphs. Actors are currently rendered as font/OAM glyphs on top of the terrain; they should move to sprite definitions after the background tile lookup is stable.

### Terrain lookup draft

| Current source | Current glyph | Semantic role | Walkable today? | Exterior draft tile symbol | Interior draft tile symbol | Notes |
| --- | --- | --- | --- | --- | --- | --- |
| `TILE_FLOOR` | `.` | Default walkable terrain | yes | `EXTERIOR_TILE_GRASS` for field/forest, `EXTERIOR_TILE_PATH` where a road overlay is later authored | `INTERIOR_TILE_FLOOR` | This is the most important default fallback because out-of-bounds draw fill currently uses floor. |
| `TILE_WALL` | `#` | Solid border, forest trees, mountain/castle walls | no | `EXTERIOR_TILE_TREE_WALL` for forest, `EXTERIOR_TILE_CLIFF_OR_STONE_WALL` for mountain/castle, `EXTERIOR_TILE_FENCE_OR_EDGE` for town border | `INTERIOR_TILE_WALL` | Start with one `EXTERIOR_TILE_WALL` symbol if per-scene wall variants are too much for the first slice. |
| `TILE_EXIT` | `>` | Map transition gate | yes, resolves map change | `EXTERIOR_TILE_EXIT_GATE` | `INTERIOR_TILE_DOOR` | Current code displays `>` even for exits whose `tile_char` is `<`; directional visual variants can be a later refinement. |
| `TILE_BUILDING` | `B` | Solid town/castle building mass | no | `EXTERIOR_TILE_BUILDING_WALL` or `EXTERIOR_TILE_ROOF` | `INTERIOR_TILE_SOLID_PROP` | Used by town and castle terrain generation for blocked structures. |

### Directional exit refinement draft

| Scene exit `tile_char` | Current renderer behavior | Future visual symbol | Exterior draft tile symbol | Interior draft tile symbol |
| --- | --- | --- | --- | --- |
| `>` | Rendered as `>` through `TILE_EXIT` | east/north/forward gate or doorway | `EXTERIOR_TILE_GATE_FORWARD` | `INTERIOR_TILE_DOOR_FORWARD` |
| `<` | Also rendered as `>` today | return/back gate or doorway | `EXTERIOR_TILE_GATE_BACK` | `INTERIOR_TILE_DOOR_BACK` |

Do not implement this refinement by changing collision. The tile remains `TILE_EXIT`; the exit table supplies target scene and spawn data.

### Actor and object glyph draft

| Current glyph | Current entity/content | Current rendering path | Draft visual target | Notes |
| --- | --- | --- | --- | --- |
| `@` | Player | OAM sprite using `assets/player_demo.png` | Keep as player sprite; later replace with final hero sprite sheet | Not a background tile. |
| `E` | Slime hostile actor | Hostile actor OAM/font glyph | Slime enemy sprite | Current semantic entity is `SLIME`; do not identify by glyph in tests. |
| `V` | Bat hostile actor | Hostile actor OAM/font glyph | Bat enemy sprite | Current semantic entity is `BAT`. |
| `L` | Lord of Slimes hostile actor | Hostile actor OAM/font glyph | Boss sprite | Current semantic entity is `LORD_OF_SLIMES`. |
| `M` | Mayor or Merchant | Non-hostile actor background/font glyph | NPC sprite variant | The same glyph represents multiple semantic entities; sprite selection must use entity ID, not glyph alone. |
| `G` | Guard | Non-hostile actor background/font glyph | Guard NPC sprite | Blocking/interactable actor. |
| `S` | Shopkeeper | Non-hostile actor background/font glyph | Shopkeeper NPC sprite | Current tool metadata names this as dialogue-like, while game content uses shop interaction; preserve semantic interaction from game content. |
| `W` | Wizard | Non-hostile actor background/font glyph | Wizard NPC sprite | Save interaction actor. |
| `?` | Amulet pickup | Non-hostile actor background/font glyph | Item sparkle/chest/amulet sprite or prop tile | Should remain interactable and observable through the actor/entity system. |

### Suggested first C symbols

Use symbolic names in the first code draft even before the exact PNG tile coordinates are finalized:

```c
#define EXTERIOR_TILE_GRASS          0u
#define EXTERIOR_TILE_WALL           1u
#define EXTERIOR_TILE_EXIT_GATE      2u
#define EXTERIOR_TILE_BUILDING_WALL  3u

#define INTERIOR_TILE_FLOOR          0u
#define INTERIOR_TILE_WALL           1u
#define INTERIOR_TILE_DOOR           2u
#define INTERIOR_TILE_SOLID_PROP     3u
```

These numeric values are placeholders. Replace them with generated or catalog-backed constants after choosing exact tile coordinates from `RPG_exterior.png` and `RPG_interior.png`.

### Minimal first implementation table

```c
static const TileGlyphEntry exterior_glyph_tiles[] = {
    { '.', EXTERIOR_TILE_GRASS, 0 },
    { '#', EXTERIOR_TILE_WALL, 0 },
    { '>', EXTERIOR_TILE_EXIT_GATE, 0 },
    { 'B', EXTERIOR_TILE_BUILDING_WALL, 0 }
};

static const TileGlyphEntry interior_glyph_tiles[] = {
    { '.', INTERIOR_TILE_FLOOR, 0 },
    { '#', INTERIOR_TILE_WALL, 0 },
    { '>', INTERIOR_TILE_DOOR, 0 },
    { 'B', INTERIOR_TILE_SOLID_PROP, 0 }
};
```

This minimal table intentionally matches the current terrain glyph vocabulary exactly. New glyphs such as `~`, `=`, `C`, or decorative `D` should be introduced only when the scene data starts authoring those semantics explicitly.

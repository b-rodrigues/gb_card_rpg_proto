#ifndef RPG_TILE_LOOKUP_H
#define RPG_TILE_LOOKUP_H

#include <stdint.h>

#define EXTERIOR_TILE_GRASS          0u
#define EXTERIOR_TILE_WALL           1u
#define EXTERIOR_TILE_EXIT_GATE      2u
#define EXTERIOR_TILE_BUILDING_WALL  3u

#define INTERIOR_TILE_FLOOR          0u
#define INTERIOR_TILE_WALL           1u
#define INTERIOR_TILE_DOOR           2u
#define INTERIOR_TILE_SOLID_PROP     3u

#define FOREST_TILE_FLOOR            0u
#define FOREST_TILE_TREE             1u
#define FOREST_TILE_GATE             2u
#define FOREST_TILE_STUMP_TL         3u
#define FOREST_TILE_STUMP_TR         4u
#define FOREST_TILE_STUMP_BL         5u
#define FOREST_TILE_STUMP_BR         6u
#define FOREST_TILE_STUMP_MINI       7u

#define RPG_TILE_BASE_EXTERIOR       128u
#define RPG_TILE_BASE_INTERIOR       132u
#define RPG_TILE_BASE_FOREST         136u

extern const uint8_t g_rpg_world_tiles[256];

static inline uint8_t rpg_lookup_tile_id(uint8_t tileset_kind, char glyph)
{
    if (glyph == '.') return (uint8_t)(128 + (tileset_kind << 2) + 0);
    if (glyph == '#') return (uint8_t)(128 + (tileset_kind << 2) + 1);
    if (glyph == '>' || glyph == '<') return (uint8_t)(128 + (tileset_kind << 2) + 2);
    if (glyph == 'B') return (uint8_t)(128 + (tileset_kind << 2) + 3);
    if (tileset_kind == 2 && glyph >= '1' && glyph <= '4') {
        return (uint8_t)(139 + (glyph - '1'));
    }
    return 0;
}

#endif /* RPG_TILE_LOOKUP_H */

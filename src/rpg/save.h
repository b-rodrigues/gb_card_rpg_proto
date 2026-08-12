#ifndef RPG_SAVE_H
#define RPG_SAVE_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

/* Battery-backed SRAM save of the canonical GameState.  The format is
 * versioned ({magic, version, checksum, state}) so future saves can be
 * migrated; see docs/save-format.md. */

#define SAVE_MAGIC   0x5247   /* "GR" */
#define SAVE_VERSION 1

/* Persist the canonical state to SRAM.  Returns true on success. */
bool save_game(const GameState *state);

/* Restore the canonical state from SRAM.  Returns false (state untouched)
 * when no valid save is present. */
bool load_game(GameState *state);

/* True when SRAM holds a valid save for this build. */
bool save_present(void);

#endif /* RPG_SAVE_H */

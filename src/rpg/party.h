#ifndef RPG_PARTY_H
#define RPG_PARTY_H

#include <stdint.h>
#include <stdbool.h>
#include "rpg/state.h"

void party_init(PartyState *party);

/* Look up a party member by character id, or NULL. */
CharacterState *party_get_member(PartyState *party, CharacterId id);
const CharacterState *party_get_member_const(const PartyState *party, CharacterId id);

bool party_add_member(PartyState *party, CharacterId id, uint8_t level,
                      uint16_t experience, uint8_t hp, uint8_t max_hp);

void party_set_hp(PartyState *party, CharacterId id, uint8_t hp);
uint8_t party_get_hp(const PartyState *party, CharacterId id);

#endif /* RPG_PARTY_H */

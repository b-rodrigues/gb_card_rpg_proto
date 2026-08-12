#include "rpg/party.h"
#include <stddef.h>

void party_init(PartyState *party)
{
    uint8_t i;
    if (!party) return;
    party->count = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        party->members[i].id = CHARACTER_NONE;
        party->members[i].hp = 0;
        party->members[i].max_hp = 0;
    }
}

CharacterState *party_get_member(PartyState *party, CharacterId id)
{
    uint8_t i;
    if (!party) return NULL;
    for (i = 0; i < party->count; i++) {
        if (party->members[i].id == id) {
            return &party->members[i];
        }
    }
    return NULL;
}

const CharacterState *party_get_member_const(const PartyState *party, CharacterId id)
{
    uint8_t i;
    if (!party) return NULL;
    for (i = 0; i < party->count; i++) {
        if (party->members[i].id == id) {
            return &party->members[i];
        }
    }
    return NULL;
}

void party_set_hp(PartyState *party, CharacterId id, uint8_t hp)
{
    CharacterState *m = party_get_member(party, id);
    if (!m) return;
    m->hp = hp;
}

uint8_t party_get_hp(const PartyState *party, CharacterId id)
{
    const CharacterState *m = party_get_member_const(party, id);
    return m ? m->hp : 0;
}

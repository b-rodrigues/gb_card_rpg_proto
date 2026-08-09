#include "combatant.h"

void combatant_init(Combatant *c, const char *name, uint8_t hp, uint8_t max_hp)
{
    if (!c) return;
    c->name = name;
    c->hp = hp;
    c->max_hp = max_hp;
}

void combatant_take_damage(Combatant *c, uint8_t damage)
{
    if (!c) return;
    if (damage >= c->hp) {
        c->hp = 0;
    } else {
        c->hp -= damage;
    }
}

bool combatant_is_dead(const Combatant *c)
{
    if (!c) return true;
    return c->hp == 0;
}

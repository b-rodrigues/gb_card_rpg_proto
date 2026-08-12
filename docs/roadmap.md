# RPG State Foundation v1 — Implementation Plan

## 1. Objective — DONE

Implement a reusable, Game Boy-friendly RPG state layer that becomes the canonical source of persistent gameplay state.

The system should answer:

* Where is the player?
* Who is in the party?
* What items does the party possess?
* Which story/world flags have been set?
* What numeric progression variables exist?
* What persistent changes have happened to the world?

It should **not** yet implement quests, scripting, save hardware, sophisticated character progression, or the card system.

The guiding principle is:

> **Store state generically; keep game rules in systems that operate on that state.**

The resulting architecture should be usable by the current prototype and plausibly reusable by a future Game Boy RPG.

---

# 2. Architectural Target — DONE

The target architecture is:

```text
                    GameState
                       │
       ┌───────────────┼────────────────┐
       │               │                │
       ▼               ▼                ▼
 SceneState       PartyState       WorldState
       │               │                │
       │               ├── CharacterState[]
       │               │
       │               └── InventoryState
       │
       └──────────────────────────────────
                       │
                       ▼
              ProgressionState
                ├── Flags
                └── Variables
```

More concretely:

```text
GameState
├── SceneState
├── PartyState
├── InventoryState
├── FlagState
├── VariableState
└── WorldState
```

Keep these structures **plain C structs**.

Do not introduce classes, dynamic allocation, generic containers, reflection, scripting languages, or an ECS.

---

# 3. Establish the Core `GameState` — DONE

Create something along the lines of:

```c
typedef struct {
    SceneState scene;
    PartyState party;
    InventoryState inventory;
    FlagState flags;
    VariableState variables;
    WorldState world;
} GameState;
```

There should be exactly one authoritative game state for the running game.

The existing game systems should gradually stop owning their own duplicated representations of persistent state.

For example, avoid having:

```text
game.c        → current scene
world.c       → current scene
debug.c       → current scene
```

Instead:

```text
GameState.scene.scene_id
```

becomes authoritative.

---

# 4. Scene State — DONE

Create:

```c
typedef struct {
    SceneId scene_id;
    uint8_t player_x;
    uint8_t player_y;
    uint8_t player_facing;
} SceneState;
```

This represents **runtime state**, not the definition of the scene.

The distinction must remain:

```text
SceneDefinition
    static content
    map
    terrain
    actors
    exits
    music

SceneState
    mutable state
    current scene
    player position
    player facing
```

Do not put scene maps inside `GameState`.

Do not duplicate `SceneDefinition` data inside `SceneState`.

---

# 5. Party State — DONE

Introduce a small party model.

Initially:

```c
#define MAX_PARTY_MEMBERS 4

typedef struct {
    uint8_t id;
    uint8_t level;
    uint16_t experience;
    uint16_t hp;
    uint16_t max_hp;
} CharacterState;

typedef struct {
    CharacterState members[MAX_PARTY_MEMBERS];
    uint8_t count;
} PartyState;
```

The exact fields can be adjusted to match the current combat system.

### Important distinction

Keep character **definitions** separate from character **state**.

For example:

```text
CharacterDefinition
    name
    base stats
    visual
    abilities

CharacterState
    id
    level
    XP
    HP
```

Don't create `CharacterDefinition` yet if the existing prototype doesn't need it.

The important architectural rule is simply:

> Static identity/content must not be confused with mutable runtime state.

---

# 6. Inventory State — DONE

Introduce a minimal inventory representation.

```c
#define MAX_INVENTORY_ITEMS 32

typedef struct {
    ItemId item_id;
    uint8_t quantity;
} InventoryEntry;

typedef struct {
    InventoryEntry entries[MAX_INVENTORY_ITEMS];
    uint8_t count;
} InventoryState;
```

Provide operations such as:

```c
bool inventory_add(InventoryState *inventory,
                   ItemId item_id,
                   uint8_t quantity);

bool inventory_remove(InventoryState *inventory,
                      ItemId item_id,
                      uint8_t quantity);

uint8_t inventory_count(const InventoryState *inventory,
                        ItemId item_id);
```

### Important constraint

Do **not** build:

* equipment
* crafting
* durability
* item modifiers
* shops
* item scripting
* randomized item properties

Those are game features.

The foundation only needs to answer:

> Does the player possess this item, and how many?

---

# 7. Flags — DONE

This is one of the most important systems in the entire foundation.

Introduce a generic `FlagId`.

For example:

```c
typedef uint16_t FlagId;
```

Then a fixed-size storage representation appropriate for the Game Boy.

The public interface should conceptually provide:

```c
void game_flag_set(GameState *state, FlagId flag);
void game_flag_clear(GameState *state, FlagId flag);
bool game_flag_is_set(const GameState *state, FlagId flag);
```

Potential flags might eventually include:

```text
FLAG_MET_MAYOR
FLAG_DEFEATED_FIRST_SLIME
FLAG_CASTLE_GATE_OPEN
FLAG_INTRO_COMPLETE
```

But don't encode game-specific semantics into the flag system itself.

The state system knows:

```text
flag 37 = true
```

The game knows:

```text
flag 37 = FLAG_MET_MAYOR
```

---

# 8. Variables — DONE

Create a generic numeric variable system alongside flags.

For example:

```c
typedef uint16_t VariableId;
```

with:

```c
int16_t game_variable_get(
    const GameState *state,
    VariableId variable
);

void game_variable_set(
    GameState *state,
    VariableId variable,
    int16_t value
);

void game_variable_add(
    GameState *state,
    VariableId variable,
    int16_t amount
);
```

Potential uses:

```text
VAR_GOLD
VAR_CHAPTER
VAR_SLIMES_DEFEATED
VAR_MAYOR_DIALOGUE_STAGE
```

Again, the generic state system shouldn't care what these mean.

---

# 9. World State — DONE

This is the bridge between your new state system and the `WorldActor` architecture.

Create a small persistent-state representation for actors/world objects.

Conceptually:

```c
typedef struct {
    uint16_t actor_id;
    uint8_t state;
} PersistentActorState;
```

with a fixed-size collection.

For example:

```text
MAYOR
    state = NORMAL

SLIME_001
    state = DEFEATED

CHEST_003
    state = OPENED
```

The exact representation should be kept deliberately minimal.

## Critical distinction

Do not replace `WorldActorRuntime` with `PersistentActorState`.

Instead:

```text
WorldActorDefinition
        +
PersistentActorState
        ↓
WorldActorRuntime
```

The runtime actor is what the world needs to operate.

Persistent state is what survives scene reloads.

---

# 10. Actor IDs — DONE

This implementation will expose an important requirement:

**World actors need stable IDs.**

You need to distinguish:

```text
SLIME_001
SLIME_002
SLIME_003
```

rather than merely:

```text
SLIME
```

Otherwise you cannot eventually say:

```text
SLIME_002 = defeated
```

while:

```text
SLIME_003 = alive
```

Introduce a stable `ActorId`.

The ID should identify a particular persistent world actor, not merely its type.

Keep actor type/definition identity separate from instance identity.

---

# 11. State Initialization — DONE

Implement:

```c
void game_state_init(GameState *state);
void game_state_reset(GameState *state);
```

`game_state_init()` should establish a valid default starting state.

For example:

```text
scene = FIELD
player = starting position
party = initial party
inventory = initial inventory
flags = cleared
variables = default values
world = default world state
```

`game_state_reset()` should restore the canonical initial state.

Avoid having individual systems independently initialize pieces of the RPG state.

---

# 12. State Ownership Rules — DONE

This is important enough to document.

### `GameState` owns mutable persistent gameplay state. — DONE

### `SceneDefinition` owns static scene content. — DONE

### `WorldActorDefinition` owns static actor content. — DONE

### `WorldActorRuntime` owns temporary runtime information. — DONE

### `BattleState` owns temporary battle state. — DONE

For example:

```text
GameState
    FLAG_MET_MAYOR = true

SceneDefinition
    Mayor exists at (8,5)

WorldActorRuntime
    Mayor currently at (8,5)

BattleState
    temporary combat HP/turn/card state
```

This prevents the codebase from turning into one giant mutable structure.

---

# 13. Keep Battle State Separate — DONE

Do **not** put the entire battle system into `GameState`.

Eventually:

```c
typedef struct {
    ...
} BattleState;
```

will represent:

```text
current combatants
turn
temporary HP
battle phase
cards
damage
etc.
```

When combat ends, only the appropriate results should be written back to `GameState`.

Conceptually:

```text
GameState
    ↓
enter battle
    ↓
BattleState created
    ↓
combat
    ↓
results applied
    ↓
BattleState destroyed
```

This will be especially important when the card system arrives.

---

# 14. Integrate With the Existing World — DONE

Once the structures exist, migrate the existing runtime gradually.

Do **not** rewrite everything in one commit.

Recommended order:

### Phase A — DONE

Create the state structures and unit-style operations.

No gameplay behavior changes.

### Phase B — DONE

Make `SceneState` authoritative.

### Phase C — DONE

Make party state authoritative.

### Phase D — DONE

Add inventory state.

### Phase E — DONE

Add flags/variables.

### Phase F — DONE

Connect persistent actor state.

At every phase:

```text
make test-harness
make test
```

must remain green.

---

# 15. Integrate With the Debug Harness — DONE

This is where the project becomes particularly powerful.

Extend the scenario format so a scenario can specify initial RPG state.

Initially support:

```json
{
  "initial_state": {
    "scene": "TOWN"
  }
}
```

Then gradually:

```json
{
  "initial_state": {
    "scene": "TOWN",
    "flags": {
      "MET_MAYOR": true
    },
    "variables": {
      "GOLD": 100
    }
  }
}
```

Eventually:

```json
{
  "initial_state": {
    "scene": "TOWN",
    "player": {
      "x": 8,
      "y": 6
    },
    "flags": {
      "MET_MAYOR": true,
      "CASTLE_GATE_OPEN": false
    },
    "variables": {
      "GOLD": 100
    },
    "party": {
      "HERO": {
        "level": 3,
        "hp": 24
      }
    }
  }
}
```

Don't implement every field immediately.

Build the protocol incrementally.

---

# 16. Add State Telemetry — DONE

The LLM harness needs enough information to understand state.

Whenever a scenario starts, telemetry/debug output should be able to expose something like:

```text
STATE
scene=TOWN
player=(8,6)
flags=MET_MAYOR
gold=100
party_count=1
```

More importantly, state-changing operations should emit meaningful events.

For example:

```text
FLAG_SET MET_MAYOR
VARIABLE_SET GOLD 150
ITEM_ADD POTION 1
SCENE_CHANGE TOWN -> FOREST
ACTOR_STATE_CHANGE SLIME_001 DEFEATED
```

Don't dump the entire `GameState` every frame.

Telemetry should be **event-oriented**.

---

# 17. Add Assertions — DONE

The harness should eventually be able to express:

```text
assert scene == TOWN
assert flag MET_MAYOR == true
assert variable GOLD == 150
assert item POTION == 1
assert actor SLIME_001 == DEFEATED
```

This makes the RPG state system directly testable.

For the LLM, that's far better than:

> "Look at the screenshot and determine whether the Mayor seems to have disappeared."

---

# 18. Add State Snapshots — DONE

Your existing debug protocol should gain a canonical state snapshot.

The important property is:

> **The same state should be represented consistently everywhere.**

For example:

```text
GAME_STATE
scene=TOWN
player=8,6
facing=UP

PARTY
0:HERO level=3 hp=24/30 xp=120

INVENTORY
POTION=3
GOLD=150

FLAGS
MET_MAYOR
INTRO_COMPLETE

VARIABLES
CHAPTER=1

WORLD
SLIME_001=DEFEATED
```

This should be optimized for **LLM readability**, not human graphical debugging.

---

# 19. Add Regression Scenarios — DONE

I'd specifically create scenarios for:

### State initialization — DONE

```text
fresh game
→ expected default state
```

### Flag operations — DONE

```text
set flag
→ assert true
clear flag
→ assert false
```

### Variables — DONE

```text
set
add
read
```

### Inventory — DONE

```text
add item
remove item
quantity
```

### Party — DONE

```text
initial member
level
HP
XP
```

### Actor persistence — DONE

```text
defeat actor
leave scene
return
→ actor remains defeated
```

### Scene persistence — DONE

```text
move to scene
change state
leave
return
→ state preserved
```

### Complete state fixture — DONE

Create one scenario that initializes a nontrivial `GameState` and verifies the whole snapshot.

That scenario becomes extremely valuable as the architecture evolves.

---

# 20. Save/Load Preparation — But Not Implementation Yet — DONE

We should design `GameState` so that it can eventually be serialized.

Don't implement battery RAM/save slots yet.

But establish:

> **If a piece of state is part of `GameState`, it is potentially saveable.**

Conversely:

> **If something is temporary runtime state, it should not automatically become part of the save format.**

This gives us a clean future boundary:

```text
GameState
    ↓
Save serialization
    ↓
SRAM
```

The save system can come later.

---

# 21. Memory Constraints — DONE

Because this is Game Boy development, the implementation must remain deliberately fixed-size.

Avoid:

```c
malloc()
calloc()
realloc()
```

for the RPG state layer.

Prefer:

```c
#define MAX_PARTY_MEMBERS 4
#define MAX_INVENTORY_ITEMS 32
#define MAX_FLAGS ...
#define MAX_VARIABLES ...
#define MAX_PERSISTENT_ACTORS ...
```

The exact limits should be chosen based on actual ROM/RAM requirements.

The important thing is that memory usage is predictable.

We should also keep the state structures small enough that copying `GameState` is not accidentally expensive.

---

# 22. File Organization — DONE

I'd suggest something along these lines:

```text
src/rpg/
    state.c
    state.h
    party.c
    party.h
    inventory.c
    inventory.h
    progression.c
    progression.h
    world_state.c
    world_state.h
```

Potentially:

```text
src/rpg/state.h
```

owns the top-level `GameState`.

Don't split every tiny structure into its own file just for the sake of organization.

The objective is discoverability.

An agent should be able to answer:

> "Where is persistent RPG state defined?"

by opening:

```text
src/rpg/state.h
```

---

# 23. Update `AGENTS.md` — DONE

Once implemented, add explicit rules such as:

### Canonical state — DONE

> `GameState` is the authoritative source for persistent gameplay state. Do not create parallel persistent representations in individual systems.

### Definition/runtime separation — DONE

> Static definitions must not be used as mutable persistent state.

### Temporary state — DONE

> Battle/UI/input state must remain separate from persistent `GameState` unless it represents a gameplay result that survives the temporary state.

### Debugging — DONE

> New persistent gameplay state must be observable through the debug protocol and injectable through scenarios where practical.

This is particularly important given that the project is being developed by LLM agents.

---

# 24. Update `DEBUG_PROTOCOL.md` — DONE

Add sections for:

```text
STATE INITIALIZATION
STATE SNAPSHOT
FLAGS
VARIABLES
INVENTORY
PARTY
PERSISTENT ACTORS
STATE ASSERTIONS
```

The protocol should explain both:

1. how an agent requests/sets state;
2. how the game reports state back.

The protocol should remain **semantic**, not implementation-specific.

For example:

Good:

```text
FLAG_SET MET_MAYOR
```

Less useful:

```text
RAM[0xC042] = 0x01
```

The latter can exist as low-level diagnostics, but should not be the primary agent interface.

---

# 25. Definition of Done — DONE

I would consider RPG State Foundation v1 complete when:

* `GameState` is the canonical persistent state.
* Scene/player state is represented by `SceneState`.
* Party state exists independently of battle state.
* Inventory state exists.
* Generic flags exist.
* Generic variables exist.
* Persistent actor state has a defined representation.
* World actors have stable IDs.
* Battle state remains separate.
* State can be initialized deterministically.
* The debug harness can inject core state.
* The debug harness can inspect core state.
* State changes emit useful telemetry.
* State assertions work.
* At least ~10–15 regression scenarios exercise the system.
* `make test-harness` passes.
* `make test` passes.
* The release ROM still builds.
* mGBA remains the primary development/debug validation environment.
* Gambatte/SameBoy compatibility checks continue to work where relevant.

And critically:

> **No gameplay feature should have to know how the underlying state is stored.**

---

# What comes immediately after

Once this is complete, I'd resist jumping directly into cards.

The natural sequence becomes:

```text
                    CURRENT
                       │
                       ▼
              RPG State Foundation
                       │
          ┌────────────┼────────────┐
          ▼            ▼            ▼
       Items       Progression    Events
          │            │            │
          └────────────┼────────────┘
                       ▼
                  Save / Load
                       │
                       ▼
                Battle Expansion
                       │
                       ▼
                  Card System
```

That gives us a very strong RPG substrate before introducing the most game-specific part of your original idea.

---

# Roadmap

## 1. RPG State Foundation v1 — DONE

Create the canonical `GameState` with scene, party, inventory, flags, variables, and persistent world state. Integrate it with the deterministic debug harness so an LLM can construct, inspect, and assert arbitrary RPG states.

## 2. Items & Inventory — DONE

Build the first genuinely reusable item system on top of `InventoryState`, starting with simple consumables and leaving equipment/crafting for later.

## 3. Progression — DONE

Add character XP, leveling, stats, and other progression primitives without baking a particular game's progression rules into the foundation.

## 4. Scripted Events — DONE

Build a lightweight event system using scenes, actors, flags, variables, dialogue, and transitions — enough to express RPG sequences without creating a general-purpose scripting language.

## 5. Foundation Reusability (content decoupling) — DONE

The vertical slice (Mayor quest, sword, boss, ending) is a complete RPG, so the
question became whether the architecture is reusable for a second game rather
than a pile of special cases.  All game content now lives in `src/game/` and is
registered with a generic engine:

* `src/game/game_ids.h` — named story-flag/variable/currency semantics (the
  engine stores generic slots; the game names them).
* `src/game/events.c`, `dialogue.c`, `actors.c`, `items.c` — the content tables,
  registered via `event_init` / `dialogue_register` / `actor_register_tables` /
  `item_register_defs`.
* `src/game/shops.{h,c}` — per-shop stock lists (`shop_id` on the actor def).
* `src/game/content.c` — `game_new_game` (initial state), `game_screen_after_victory`
  (the battle screen no longer knows about endings), and the moved
  `game_hero_attack` / `game_on_level_up` hooks.
* `state.c` is fully generic: `game_state_zero`; the starting state moved out.

Removed hard-coded game bits: the `actor_enemy_name` switch (enemy names now live
on the actor definitions as `display_name`), the shop's `ITEM_POTION` hardcode,
and the `ENDING_SHOWN` check in `battle_screen.c`.  Verified behavior-neutral:
82/82 scenarios, `make lint`, `make test`, and the save-boundary roundtrip.

## 6. Second Quest — Proving the Abstraction — DONE

The Lost Merchant quest (fetch/deliver/unlock) is structurally different from the
kill-counter: find the Amulet in the Forest, deliver it to the Merchant in Town,
receive a gold reward, and unlock his shop (which sells NUT).  Built entirely
from game data — event entries, dialogue, item defs, actor defs, a shop stock
list — with no game-specific branches in the engine.

The second quest forced three generic engine primitives, each expressed as data
and reusable by any game:

* `EVENT_COND_ITEM_COUNT` — an event condition on inventory count;
* `EVENT_ACTION_ADD_CURRENCY` / `EVENT_ACTION_REMOVE_ITEM` — reward/take actions;
* `shop_id` on `WorldActorDefinition` — per-actor shops.

New scenarios: `merchant_quest_start`, `merchant_pickup`, `merchant_deliver`,
`merchant_shop_open`, `merchant_repeat` (87 scenarios total).

## 7. Save / Load

Serialize the persistent `GameState` into Game Boy-compatible save storage, with deterministic harness tests for save/load round trips.

## 8. Battle Expansion — PARTIAL

Strengthen the existing battle system now that it has a proper persistent RPG state to interact with.
Missing:
* per-enemy attack/stats and AI (enemy attack is still hardcoded to 2; all enemies hit identically);
* damage variance / critical hits (the deterministic RNG is unused in battle);
* defense and stats beyond HP/attack;
* a battle command menu (attack/skill/item/flee) instead of the current A/B/START shortcuts;
* multiple enemies / groups and speed-based turn order;
* status effects;
* richer rewards (XP/leveling from battle, loot);
* flee chance / consequences.

## 9. Card System

Finally introduce the Baten Kaitos-inspired card mechanics on top of the stable RPG/battle foundation rather than allowing the card system to dictate the architecture.

**Guiding principle for the whole roadmap:** build only the abstractions that the current RPG actually proves it needs, while making every important state deterministic, observable, and testable by an LLM.

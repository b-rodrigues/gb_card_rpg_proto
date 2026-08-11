# RPG State Foundation v1 — Implementation Plan

## 1. Objective

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

# 2. Architectural Target

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

# 3. Establish the Core `GameState`

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

# 4. Scene State

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

# 5. Party State

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

# 6. Inventory State

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

# 7. Flags

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

# 8. Variables

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

# 9. World State

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

# 10. Actor IDs

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

# 11. State Initialization

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

# 12. State Ownership Rules

This is important enough to document.

### `GameState` owns mutable persistent gameplay state.

### `SceneDefinition` owns static scene content.

### `WorldActorDefinition` owns static actor content.

### `WorldActorRuntime` owns temporary runtime information.

### `BattleState` owns temporary battle state.

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

# 13. Keep Battle State Separate

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

# 14. Integrate With the Existing World

Once the structures exist, migrate the existing runtime gradually.

Do **not** rewrite everything in one commit.

Recommended order:

### Phase A

Create the state structures and unit-style operations.

No gameplay behavior changes.

### Phase B

Make `SceneState` authoritative.

### Phase C

Make party state authoritative.

### Phase D

Add inventory state.

### Phase E

Add flags/variables.

### Phase F

Connect persistent actor state.

At every phase:

```text
make test-harness
make test
```

must remain green.

---

# 15. Integrate With the Debug Harness

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

# 16. Add State Telemetry

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

# 17. Add Assertions

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

# 18. Add State Snapshots

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

# 19. Add Regression Scenarios

I'd specifically create scenarios for:

### State initialization

```text
fresh game
→ expected default state
```

### Flag operations

```text
set flag
→ assert true
clear flag
→ assert false
```

### Variables

```text
set
add
read
```

### Inventory

```text
add item
remove item
quantity
```

### Party

```text
initial member
level
HP
XP
```

### Actor persistence

```text
defeat actor
leave scene
return
→ actor remains defeated
```

### Scene persistence

```text
move to scene
change state
leave
return
→ state preserved
```

### Complete state fixture

Create one scenario that initializes a nontrivial `GameState` and verifies the whole snapshot.

That scenario becomes extremely valuable as the architecture evolves.

---

# 20. Save/Load Preparation — But Not Implementation Yet

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

# 21. Memory Constraints

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

# 22. File Organization

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

# 23. Update `AGENTS.md`

Once implemented, add explicit rules such as:

### Canonical state

> `GameState` is the authoritative source for persistent gameplay state. Do not create parallel persistent representations in individual systems.

### Definition/runtime separation

> Static definitions must not be used as mutable persistent state.

### Temporary state

> Battle/UI/input state must remain separate from persistent `GameState` unless it represents a gameplay result that survives the temporary state.

### Debugging

> New persistent gameplay state must be observable through the debug protocol and injectable through scenarios where practical.

This is particularly important given that the project is being developed by LLM agents.

---

# 24. Update `DEBUG_PROTOCOL.md`

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

# 25. Definition of Done

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

## 1. RPG State Foundation v1

Create the canonical `GameState` with scene, party, inventory, flags, variables, and persistent world state. Integrate it with the deterministic debug harness so an LLM can construct, inspect, and assert arbitrary RPG states.

## 2. Items & Inventory

Build the first genuinely reusable item system on top of `InventoryState`, starting with simple consumables and leaving equipment/crafting for later.

## 3. Progression

Add character XP, leveling, stats, and other progression primitives without baking a particular game's progression rules into the foundation.

## 4. Scripted Events

Build a lightweight event system using scenes, actors, flags, variables, dialogue, and transitions — enough to express RPG sequences without creating a general-purpose scripting language.

## 5. Save / Load

Serialize the persistent `GameState` into Game Boy-compatible save storage, with deterministic harness tests for save/load round trips.

## 6. Battle Expansion

Strengthen the existing battle system now that it has a proper persistent RPG state to interact with.

## 7. Card System

Finally introduce the Baten Kaitos-inspired card mechanics on top of the stable RPG/battle foundation rather than allowing the card system to dictate the architecture.

**Guiding principle for the whole roadmap:** build only the abstractions that the current RPG actually proves it needs, while making every important state deterministic, observable, and testable by an LLM.
. I’d make this the next formal milestone and keep it deliberately smaller than a full RPG framework. The goal is to establish **canonical state and clean boundaries**, then prove those boundaries through the existing harness.

# RPG State Foundation v1 — Implementation Plan

## 1. Objective

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

# 2. Architectural Target

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

# 3. Establish the Core `GameState`

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

# 4. Scene State

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

# 5. Party State

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

# 6. Inventory State

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

# 7. Flags

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

# 8. Variables

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

# 9. World State

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

# 10. Actor IDs

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

# 11. State Initialization

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

# 12. State Ownership Rules

This is important enough to document.

### `GameState` owns mutable persistent gameplay state.

### `SceneDefinition` owns static scene content.

### `WorldActorDefinition` owns static actor content.

### `WorldActorRuntime` owns temporary runtime information.

### `BattleState` owns temporary battle state.

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

# 13. Keep Battle State Separate

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

# 14. Integrate With the Existing World

Once the structures exist, migrate the existing runtime gradually.

Do **not** rewrite everything in one commit.

Recommended order:

### Phase A

Create the state structures and unit-style operations.

No gameplay behavior changes.

### Phase B

Make `SceneState` authoritative.

### Phase C

Make party state authoritative.

### Phase D

Add inventory state.

### Phase E

Add flags/variables.

### Phase F

Connect persistent actor state.

At every phase:

```text
make test-harness
make test
```

must remain green.

---

# 15. Integrate With the Debug Harness

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

# 16. Add State Telemetry

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

# 17. Add Assertions

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

# 18. Add State Snapshots

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

# 19. Add Regression Scenarios

I'd specifically create scenarios for:

### State initialization

```text
fresh game
→ expected default state
```

### Flag operations

```text
set flag
→ assert true
clear flag
→ assert false
```

### Variables

```text
set
add
read
```

### Inventory

```text
add item
remove item
quantity
```

### Party

```text
initial member
level
HP
XP
```

### Actor persistence

```text
defeat actor
leave scene
return
→ actor remains defeated
```

### Scene persistence

```text
move to scene
change state
leave
return
→ state preserved
```

### Complete state fixture

Create one scenario that initializes a nontrivial `GameState` and verifies the whole snapshot.

That scenario becomes extremely valuable as the architecture evolves.

---

# 20. Save/Load Preparation — But Not Implementation Yet

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

# 21. Memory Constraints

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

# 22. File Organization

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

# 23. Update `AGENTS.md`

Once implemented, add explicit rules such as:

### Canonical state

> `GameState` is the authoritative source for persistent gameplay state. Do not create parallel persistent representations in individual systems.

### Definition/runtime separation

> Static definitions must not be used as mutable persistent state.

### Temporary state

> Battle/UI/input state must remain separate from persistent `GameState` unless it represents a gameplay result that survives the temporary state.

### Debugging

> New persistent gameplay state must be observable through the debug protocol and injectable through scenarios where practical.

This is particularly important given that the project is being developed by LLM agents.

---

# 24. Update `DEBUG_PROTOCOL.md`

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

# 25. Definition of Done

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

## 1. RPG State Foundation v1

Create the canonical `GameState` with scene, party, inventory, flags, variables, and persistent world state. Integrate it with the deterministic debug harness so an LLM can construct, inspect, and assert arbitrary RPG states.

## 2. Items & Inventory

Build the first genuinely reusable item system on top of `InventoryState`, starting with simple consumables and leaving equipment/crafting for later.

## 3. Progression

Add character XP, leveling, stats, and other progression primitives without baking a particular game's progression rules into the foundation.

## 4. Scripted Events

Build a lightweight event system using scenes, actors, flags, variables, dialogue, and transitions — enough to express RPG sequences without creating a general-purpose scripting language.

## 5. Save / Load

Serialize the persistent `GameState` into Game Boy-compatible save storage, with deterministic harness tests for save/load round trips.

## 6. Battle Expansion

Strengthen the existing battle system now that it has a proper persistent RPG state to interact with.

## 7. Card System

Finally introduce the Baten Kaitos-inspired card mechanics on top of the stable RPG/battle foundation rather than allowing the card system to dictate the architecture.

**Guiding principle for the whole roadmap:** build only the abstractions that the current RPG actually proves it needs, while making every important state deterministic, observable, and testable by an LLM.

# Foundation Contract

This repository is a **reusable Game Boy RPG runtime with a deliberately thin
game/content layer**.  It is the *foundation*, not a game.  The rule below is
the contract every contributor and agent must respect.

## This repository provides

* Game Boy runtime (custom CRT0, MBC5, timer-driven audio, deterministic
  input)
* RPG persistent state (`GameState`): scene, party, inventory, flags,
  variables, currency, persistent world actors, progression, equipment
* World / scenes / exits / entities / actors (data-driven)
* Dialogue, scripted events, quests (all registered content)
* Items, inventory mechanics, equipment, shops
* Battle foundation (combatants, turn flow, results back to `GameState`)
* Battery-backed save/load of `GameState` (versioned format)
* Audio (music tracks, hardware-timer ISR)
* UI (menu frame, HUD) and screens (overworld, battle, dialogue, item, shop,
  game-over, ending, thanks)
* Deterministic debug harness: scenarios, input injection, frame stepping,
  telemetry, semantic snapshots, assertions, RNG control
* Asset pipeline (design/spec only until the graphics milestone)
* Reproducible build + memory budget (`make memmap`)

## This repository does not provide

* A specific game story
* Specific maps / scenes
* Specific characters, enemies, or bosses
* Specific card mechanics
* Specific art, tiles, sprites, or palettes
* Specific music
* Specific progression rules
* Specific quest structures beyond the linear 0/1/2 status model

Those belong in a game repository built on this foundation (see
`docs/game-vs-engine.md`).

## The golden rule

> **A feature is added to the foundation only when it is demonstrably
> reusable or necessary for the runtime itself.**

In practice:

* New enemies, items, quests, shops, dialogue lines, or scene content are
  **`src/game/` changes only** — never engine changes.
* Engine changes are justified by a *second use case*, not by anticipation.
  (Example: the Lost Merchant quest proved `EVENT_COND_ITEM_COUNT` and the
  currency/item actions were worth promoting.)
* Do not add machinery for known gaps (multi-stage quests, key-locked
  locations, card mechanics, crafting, status effects, ECS, scripting
  languages) until a real game feature demonstrates the need.
* When a game feature needs something the foundation lacks, implement it in
  the game repository first; promote it back **only if a second use case**
  proves it belongs in the runtime.

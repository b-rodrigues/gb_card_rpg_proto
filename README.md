# Pretium Pacis

A Game Boy RPG about **survival, travel, exploration, combat, money, and political intrigue**.

`Pretium Pacis` — *the price of peace* — is the working title for the RPG that this repository is becoming the technical foundation for.

The project targets authentic **Nintendo Game Boy (DMG) and Game Boy Color (CGB)** constraints. The goal is not to reproduce every convention of a traditional JRPG, but to remove the parts of RPGs that tend to feel like chores and replace them with compact, decision-focused systems.

> **The player should spend their time making interesting decisions, not walking between menus and locations just because RPGs traditionally make them do so.**

## The Game

The setting is a small principality caught in a political crisis.

A large empire has imposed a blockade on the principality, ostensibly to protect a minority population living there. The blockade disrupts trade and creates economic and political pressure without necessarily becoming an open war.

The player is not initially a chosen hero or great military leader. They are essentially **a person trying to make a living**.

They can take guild jobs, explore dangerous places, trade, invest their money, and gradually become involved in the conflict. Eventually, a former friend may approach them about joining a paramilitary organization secretly financed by the principality: a force that can attack imperial interests while the principality maintains plausible deniability.

From that point on, the player has substantially greater freedom to use their wages, loot, investments, and knowledge of the economy to shape their own fortunes and, indirectly, the wider conflict.

The story is therefore driven by the intersection of:

- personal survival;
- political conflict;
- economic pressure;
- exploration;
- violence and its consequences;
- and the player's growing ability to influence the world.

## Core Design Principles

### No pointless walking

Towns are not intended to be large maps full of buildings that the player has to enter individually.

A town is primarily a **list of things to do**:

```text
TOWN

> Guild
  Work
  Market
  Bank
  Investments
  Inn
  Leave
```

The player makes decisions from the list instead of spending time walking around town looking for the one NPC or building they need.

### No tedious overworld travel

Travel between settlements and destinations is its own gameplay system.

Before a journey, the player prepares:

- supplies;
- money;
- equipment;
- route;
- and possibly a guide.

The player can then choose between different ways of traveling.

**Travel on foot** is cheap but risky.

**Hire a guide with a horse** is expensive but considerably safer.

The journey is handled as a compact, Oregon Trail-style sequence of decisions and events rather than as hours of walking across an empty overworld.

### Exploration is physical

Once the player reaches a dungeon, cave, temple, mine, or other destination, exploration switches to a traditional top-down Game Boy map.

The player controls the character directly with the D-pad.

Enemies exist on the map and move around. The player can therefore:

- approach enemies;
- avoid them;
- position themselves advantageously;
- or deliberately engage them.

Exploration is therefore about **navigation and decisions**, rather than random encounters every few steps.

### Battles are card-based

When combat begins, the game transitions into a **Baten Kaitos-inspired card battle system**.

The battle system is intended to make encounters strategic without requiring a huge number of commands or complex real-time controls.

The current repository already contains the battle lifecycle and combat boundaries needed to plug in the eventual card mechanics. The card system itself is still under development.

### Encounters are decisions, not just fights

Not every enemy encounter has to become a battle.

Human enemies can potentially:

- threaten the player;
- demand money or resources;
- be bribed;
- be negotiated with;
- or be fought.

The player can sometimes choose to comply and live another day.

This means that an encounter can become a resource-management or role-playing decision rather than automatically becoming combat.

### Defeat is not Game Over

The game is designed around a **constant-flow structure** rather than traditional Game Over → Load Save gameplay.

The game should auto-save important persistent state.

If the player loses a battle, they do not simply see a Game Over screen. Instead, they wake up in the hospital of the last town they visited, having lost some money and/or resources.

This makes defeat a setback rather than a hard stop.

It also gives the economy an important role: a prudent player can maintain savings and investments, while an unlucky player can recover by taking short-term work.

## Activities and Making a Living

The player needs things to do that are smaller than a major expedition.

Activities provide short, optional ways to spend time and earn money or other benefits.

Examples include:

- teaching swordplay;
- helping in a soup kitchen;
- working for a guild;
- guarding a caravan;
- repairing equipment;
- delivering goods;
- hunting;
- looting a nearby cave;
- clearing a small monster-infested location.

Safe work provides a **financial safety net**. It should be possible to recover from a bad expedition without turning the game into a grind, while still making poverty meaningful.

Riskier short activities can reuse the real exploration, travel, encounter, and battle systems.

Activities can also change with the state of the world.

## The Economy

Money is intended to be more than a shop currency.

The player can:

- earn wages;
- save money;
- maintain an emergency reserve;
- buy equipment;
- invest in companies;
- short companies;
- react to economic news;
- and potentially manipulate the physical causes of economic outcomes.

The economy evolves naturally over time.

Companies can improve or deteriorate because of:

- resource availability;
- supply and demand;
- political events;
- trade disruptions;
- natural disasters;
- accidents;
- competition;
- government contracts;
- shortages;
- and other random events.

Story events use the same underlying economic state. For example, a tightening imperial blockade can disrupt trade, affect resource availability, hurt some companies, and benefit others.

The simulation is deliberately lightweight: it uses a small number of world resources, companies, commodity categories, and economic indices rather than attempting to model a realistic modern economy.

## Economic Intervention

One of the central systemic ideas is that **financial positions can create physical opportunities**.

For example:

```text
Short a company that raises wyverns
        ↓
Discover that its wyvern farm is vulnerable
        ↓
"Intervene in WYV Corp" becomes available
        ↓
Raid the farm
        ↓
Wyvern population decreases
        ↓
Production decreases
        ↓
Company performance falls
        ↓
Stock price falls
        ↓
The short position becomes profitable
```

The player can also do the reverse:

```text
Invest in a weapons manufacturer
        ↓
Discover that it depends on an iron mine
        ↓
The mine is occupied by monsters
        ↓
"Clear Iron Mine" becomes available
        ↓
Clear the mine
        ↓
Iron production resumes
        ↓
Weapons production improves
        ↓
Company performance improves
        ↓
Investment becomes more valuable
```

The intervention does **not** directly change the company's stock price. It changes the underlying world state, and the normal economic simulation produces the consequences.

This allows ordinary RPG actions — killing monsters, protecting a route, discovering a resource, sabotaging a facility, etc. — to interact with the financial system.

## Economic Simulation

The economic model is designed to be cheap enough for Game Boy hardware.

The world is the underlying economic substrate rather than a company itself:

```text
                 WORLD STATE
                     │
          ┌──────────┴──────────┐
          ↓                     ↓
      RESOURCES              CONDITIONS
          │                     │
          └──────────┬──────────┘
                     ↓
                 COMPANIES
                     ↓
                MARKET VALUE
                     ↓
              PLAYER POSITIONS
                     │
                     ↓
                INTERVENTIONS
                     │
                     ↓
              PHYSICAL WORLD
                     │
                     └──────────→ WORLD STATE
```

Resources are represented as simple stocks and flows:

```text
Iron stock:       820
Production:       +40/day
Consumption:      -55/day
```

Companies consume and produce resources. Their performance reacts to the state of those resources, demand, world events, and other small modifiers.

Company performance can update daily, while shop and commodity prices can be much stickier and update roughly every ten days.

Individual goods such as swords and potions do not need their own full economic simulation. They use:

```text
base item price
× economic modifier
× commodity modifier
× regional modifier
```

Prices then move gradually toward their target values rather than changing abruptly.

The result should be a small deterministic simulation that produces a surprisingly rich economy from a very small amount of state.

## Day / Night

A day/night system is still under consideration.

If implemented, time of day could change what is possible rather than simply changing the color of the screen.

For example:

- some town activities could become available at night;
- pubs and other nightlife could open;
- adult entertainment venues could become accessible;
- certain dungeon encounters could become more dangerous;
- some enemies could behave differently;
- night travel could be unavailable or considerably more dangerous.

The system will only be introduced if it creates meaningful decisions rather than adding another clock the player has to manage.

## Story Structure

The story is deliberately designed to start small and become increasingly political.

The player's initial concerns are mundane:

```text
Get work
  ↓
Make money
  ↓
Survive
  ↓
Explore
```

As the player becomes more capable:

```text
Guild work
  ↓
Rumors and information
  ↓
Economic opportunities
  ↓
Political involvement
  ↓
Paramilitary recruitment
  ↓
Operations against the Empire
```

The paramilitary group is not intended to be a conventional heroic resistance. It is financed by the principality so that the principality can deny direct involvement in attacks against the Empire.

The player can therefore become involved in actions that might be viewed very differently depending on who is telling the story: resistance, insurgency, terrorism, sabotage, or simply war by other means.

The game should avoid presenting this conflict as a simplistic good-versus-evil story.

## Technical Constraints

The game is deliberately being designed around the limitations of the original Game Boy.

Target hardware:

- Nintendo Game Boy (DMG);
- Game Boy Color (CGB).

Toolchain:

- GBDK-4 / `lcc`;
- RGBDS;
- Nix flakes;
- mGBA for primary development and debugging;
- additional emulators for compatibility testing.

The constraints are part of the design rather than merely an implementation inconvenience.

The game favors:

- compact data structures;
- deterministic simulation;
- integer arithmetic;
- discrete time steps;
- menu-driven town interfaces;
- small maps;
- reusable gameplay systems;
- and a limited number of simulated entities.

The goal is to create **the illusion of a large, reactive RPG world without actually simulating a large world**.

## Current Technical Foundation

The repository started as a Baten Kaitos-inspired card RPG prototype and has evolved into a reusable Game Boy RPG foundation.

The current foundation already provides:

- `GameState` and persistent world state;
- overworld scenes and maps;
- actors/entities;
- movement and collision;
- scene transitions;
- dialogue and scripted events;
- quests;
- items, inventory, equipment, and currency;
- battle lifecycle and combatants;
- battery-backed SRAM save/load;
- deterministic RNG and debug state;
- scenario-based testing;
- telemetry and assertions;
- emulator automation;
- memory-budget tooling;
- reproducible Nix-based builds.

The current graphical layer is still a prototype. The eventual game will use proper Game Boy graphics and sprites.

The card battle system is not yet implemented; the existing battle architecture is intended to provide the boundary into which it will be added.

## Development Philosophy

This repository is both a game project and the foundation for that game.

The architecture deliberately separates reusable RPG mechanisms from game-specific content:

```text
Foundation                  Game
────────────────────────────────────────
Actor                       Player
Actor                       NPC
Actor                       Enemy

Scene                       Town
Scene                       Cave
Scene                       Temple

Item                        Potion
Item                        Sword

Battle lifecycle            Card battle rules
Progression                 Game-specific progression
Save system                 Persistent world state
```

The project follows a simple rule:

> **Build the game first. Generalize only where the game demonstrates a reusable problem.**

It is not intended to become a general-purpose game engine.

## Development Setup

Enter the reproducible development environment:

```bash
nix develop
```

Build the release ROM:

```bash
make release
```

Build the debug ROM:

```bash
make debug
```

Run the ROM:

```bash
make run
```

Run the debug harness:

```bash
make test-harness
```

Validate the release ROM:

```bash
make test
```

Inspect the memory budget:

```bash
make memmap
```

See the existing documentation for the complete development workflow.

## Project Structure

```text
src/
├── core/       Boot glue, events, dialogue, story, quests
├── rpg/        GameState, items, inventory, currency, party, progression, save
├── world/      Scenes, actors, movement and collision
├── battle/     Battle lifecycle and combatants
├── input/      Joypad handling and debug input
├── audio/      Game Boy audio/music infrastructure
├── ui/         Screen and UI rendering
├── screens/    Individual game screens
├── debug/      Telemetry, scenarios, assertions and deterministic debug support
└── game/       Game-specific content

build/          Generated ROMs and build artifacts
docs/           Architecture and development documentation
tools/          Host-side development and emulator harness
```

## Documentation

The repository contains technical documentation covering the foundation, architecture, save system, graphics, testing, memory budget, and development harness.

The game design is being documented separately as the mechanics are developed. The major systems include:

- battle;
- exploration;
- encounters;
- travel;
- activities;
- economy;
- economic simulation;
- story and political systems.

## Status

The project is currently in the **foundation / systems prototyping phase**.

The immediate priority is not content production or sprite polish. The goal is to prove the core gameplay loops first:

1. travel and preparation;
2. exploration;
3. encounters;
4. card-based battle;
5. activities and short-term work;
6. economic simulation;
7. investment and economic intervention;
8. persistent consequences and story progression.

Once these systems work together, the project can grow into the actual game.

## Long-Term Goal

The eventual game should feel like a compact RPG where almost every system feeds another system:

```text
                 STORY / POLITICS
                       ↕
                    ECONOMY
                   ↕      ↕
            INVESTMENT   ACTIVITIES
                   ↕      ↕
                 TRAVEL
                    ↓
                EXPLORATION
                    ↓
                ENCOUNTERS
                    ↓
                  BATTLE
                    ↓
              WORLD CHANGES
                    ↓
                 ECONOMY
```

The player starts as somebody trying to get by.

By the end, they may have become an adventurer, investor, political operative, paramilitary fighter, economic manipulator — or some combination of all of them.

The central question is not simply **"Can you defeat the final boss?"**

It is:

> **What are you willing to do to survive, and what happens when you gain enough power to change the world around you?**

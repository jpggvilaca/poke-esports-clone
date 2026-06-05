# Project Guidelines

## Game Vision

The game is a classic monster-taming RPG-inspired 2D RPG about becoming the best esports trainer in the world.

Current direction:

- The user is the trainer/competitor profile. The trainer owns rating, money, trophies, story progress, and a roster.
- Player profiles are the battle units controlled by the trainer. They are the esports equivalent of classic collectible battlers.
- The trainer starts with a menu, picks a game genre, then picks the starter player profile's spec. For now, the only genre is League of Legends.
- The first story beat is a tutorial battle against the trainer's older brother. The trainer should win, but not stomp.
- The overworld is top-down 2D like classic handheld RPGs. Cities contain LAN cafes and major tournament buildings.
- LAN cafes are the equivalent of encounter zones: walking near or into one can trigger a battle.
- Battles are esports player profile vs esports player profile. Full roster/team switching comes later.
- Specs define player-profile identity, skill access, and counter matchups.
- Skills behave like battle moves: a player profile may learn many, but can equip only four active skills.
- Skills level up, player profiles level up, and trainer rating changes after battles.
- Major tournaments replace gym badges. Each major requires a minimum rating and awards a trophy.
- A nemesis/rival appears throughout the game, mocks the trainer, and uses player profiles that counter the trainer's roster.
- Buildings can later provide healing, shops, and other familiar RPG services.
- Styles are temporary sandbox mechanics. Rework them later into personality, archetype, stance, or remove them if they do not fit the player-profile model.

Do not add sponsors, item complexity, storage boxes, recruitment, or full team switching until the core trainer, roster, battle, rating, encounter, and major loop exists.

## Architecture Boundary

Godot is the presentation layer. C++ is the source of truth.

Use this flow for gameplay work:

```text
Godot UI/Input
    -> commands
GDExtension bridge
    -> translated calls
C++ core systems
    -> result structs and state snapshots
GDExtension bridge
    -> Godot dictionaries/signals/log rows
Godot UI/Audio/Animation
```

## C++ Core Rules

- Keep gameplay state, rules, validation, AI, economy, combat, progression, and match logic in C++.
- Prefer command/result APIs: state plus command produces updated state and factual result structs.
- Do not emit Godot signals or use Godot types inside pure backend systems.
- Do not make backend systems reactive/event-driven. They may mutate owned game state, then return facts about what changed.
- Keep systems small and composable: battle flow, skill use, progression, counter logic, items, economy, and match flow should be separate systems.
- `TrainerProfile` owns trainer-level save state and the roster. `PlayerProfileSystem` owns player-profile growth rules: XP, rank/evolution, passives, learned skills, and active skills.

## Godot Bridge Rules

- Keep Godot-specific types (`Array`, `Dictionary`, `String`, signals, Nodes, Resources) in the GDExtension bridge layer.
- Convert backend result structs into Godot-facing dictionaries, logs, or signals at the bridge boundary.
- Preserve Godot API compatibility when possible so UI scenes do not need to change during backend refactors.

## Godot UI Rules

- Godot owns UI, visuals, audio, animation, menus, and input collection.
- Godot may be event-driven with signals, but those signals should represent bridge/UI notifications, not core simulation architecture.
- Query read-only snapshots from C++ for current state instead of duplicating gameplay truth in GDScript.

## Data Rules

- Keep backend data as plain C++ structs for now.
- Do not move skill/item/enemy definitions to `.tres` unless we intentionally accept Godot as part of the data-authoring pipeline.
- If backend data needs to become external later, prefer a C++-owned loader for JSON, CSV, TOML, or another engine-neutral format.

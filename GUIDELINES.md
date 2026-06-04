# Project Guidelines

## Game Vision

The game is a Pokemon Red/Blue-inspired 2D RPG about becoming the best esports player in the world.

Current direction:

- The player starts with a menu, picks a game genre, then picks a spec. For now, the only genre is League of Legends.
- The first story beat is a tutorial battle against the player's older brother. The player should win, but not stomp.
- The overworld is top-down 2D like classic Pokemon. Cities contain LAN cafes and major tournament buildings.
- LAN cafes are the equivalent of grass/caves: walking near or into one can trigger a player-vs-player battle.
- Battles are always esports player vs esports player, not monsters or teams.
- Specs behave like Pokemon types/jobs: they define identity, skill access, and counter matchups.
- Skills behave like Pokemon moves: the player may learn many, but can equip only four active skills.
- Skills level up, the player levels up, and rating changes after battles.
- Major tournaments replace gym badges. Each major requires a minimum rating and awards a trophy.
- A nemesis/rival appears throughout the game, mocks the player, and uses the player's counter spec.
- Buildings can later provide healing, shops, and other Pokemon Center/Poke Mart-like services.

Do not add teams, sponsors, or item complexity until the core profile, battle, rating, encounter, and major loop exists.

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

# Project Guidelines

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

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
- If multiple player profiles participate in a winning battle, they split the battle's total player-profile XP evenly.
- Major tournaments replace gym badges. Each major requires a minimum rating and awards a trophy.
- A nemesis/rival appears throughout the game, mocks the trainer, and uses player profiles that counter the trainer's roster.
- Buildings can later provide healing, shops, and other familiar RPG services.
- Styles are temporary prototype mechanics. Rework them later into personality, archetype, stance, or remove them if they do not fit the player-profile model.

Do not add sponsors, item complexity, storage boxes, recruitment, or full team switching until the core trainer, roster, battle, rating, encounter, and major loop exists.

## Game Design Doctrine

Use these principles when deciding whether a mechanic belongs in the game.

- Design from player experience backward. For each mechanic, name the intended player feeling first, then the combat dynamic, then the rule/data change that creates it.
- Prefer simple mechanics that combine into interesting decisions. A new rule should either deepen an existing decision or create a clear new one.
- Every combat mechanic needs readable feedback: the player should know what happened, why it happened, and what they can do differently next turn.
- Balance is iterative. Add mechanics with exposed tuning values, tests for invariants, and enough event/result data to inspect outcomes later.
- Preserve counterplay. Strong effects should have costs, timing windows, target restrictions, cooldowns, or visible setup.
- Avoid hidden complexity. If a mechanic needs memory or history, surface the relevant state through UI text, icons, colors, or logs.
- Treat game feel as support and clarity, not decoration. Animation, color, log text, and sound should make decisions easier to understand.

Reference anchors:

- MDA: A Formal Approach to Game Design and Game Research: https://en.wikipedia.org/wiki/MDA_framework
- Designing Game Feel, A Survey: https://arxiv.org/abs/2011.09201
- On Video Game Balancing, Joining Player- and Data-Driven Analytics: https://arxiv.org/abs/2308.07576
- Demonstrating the Feasibility of Automatic Game Balancing: https://arxiv.org/abs/1603.03795

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
- Add skill-triggered follow-up mechanics through the `SkillUseRequest` pipeline and post-skill reaction stage, not by scattering special cases through damage, UI, or turn advancement.
- Keep shared skill-action bookkeeping in `BattleSession::ExecuteSkillAction`; player-only turn advancement and opponent response should stay outside that helper.
- Use `BattleSession::FinishNonSkillActionOpportunity` for pass, drill, stun, no-skill, and future forced-skip paths so cooldown and status timing stays consistent.
- Keep battle setup validation, team construction, active-index selection, and start-result emission as separate helpers so future match formats do not bloat `StartBattle`.
- Build battle events through `BattleEventFactory.h` helpers and keep farming/reward math in `BattleEconomySystem`.
- Add timed status effects through `BattleStatusUtils.h` instead of adding separate tick/apply branches in battle and skill systems.
- Keep `BattleSession.h` private helpers grouped by setup, combatant access, actions, statuses, turn flow, rewards, and views.
- Keep action validation in `BattleSession` so UI availability, rejected commands, and turn-consuming blocked actions share one rule path.
- `TrainerProfile` owns trainer-level save state and the roster. `PlayerProfileSystem` owns player-profile growth rules: XP, rank/evolution, passives, learned skills, and active skills.
- `BattleSession` may track battle participation and return reward facts, but profile XP is applied outside battle through `PlayerProfileSystem`.

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

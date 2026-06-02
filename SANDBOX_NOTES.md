# Temporary Console Sandbox

The C++ console exists only to exercise the small simulation systems before the
Godot UI is built. It is not the final game loop or final presentation layer.

## Keep

- `Models.h`: core state such as `Player`, `Opponent`, `Skill`, `SkillProgress`,
  `StoreItem`, `Spec`, styles, and rank tiers.
- `SimulationData.*`: temporary sample content and balance values. Replace the
  generic sample rows with real content later.
- The calculations inside `BattleSystem.cpp`: focus spending, affordable enemy
  actions, accuracy, damage, style modifiers, minimum damage, and skill XP.
- The small progression calculations in `Game.cpp`: player XP, leveling, rank
  tiers, generated opponents, recovery, store purchase, and tournament rounds.
  Move these into their own Godot-facing simulation API only when that becomes
  useful.

## Delete Or Replace When Godot UI Starts

- `main.cpp`.
- The menu loop, input helpers, and formatted console output in `Game.cpp`.
- The input helper and formatted `std::cout` text in `BattleSystem.cpp`.
- Console labels returned by `ToString(...)` in `Models.h` if Godot stores its
  own display labels.

Search for `SANDBOX UI ONLY` to find temporary console code quickly.


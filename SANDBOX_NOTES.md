# Temporary Console Sandbox

The C++ console exists only to exercise the small simulation systems before the
Godot UI is built. It is not the final game loop or final presentation layer.

## Keep

- `Models.h`: core state such as `Player`, `Opponent`, `GameType`, `Skill`,
  `SkillProgress`, `StoreItem`, `Spec`, styles, and rank tiers.
- `SimulationData.*`: temporary sample content and balance values. Replace the
  generic sample rows with real content later.
- The calculations inside `BattleSystem.cpp`: focus spending, style-filtered
  loadouts, affordable enemy actions, accuracy, damage, spec modifiers, minimum
  damage, and skill XP.
- The small progression calculations in `Game.cpp`: player XP, leveling, rank
  tiers, generated opponents, recovery, store purchase, and tournament rounds.
  Move these into their own Godot-facing simulation API only when that becomes
  useful.

## Delete Or Replace When Godot UI Starts

- `main.cpp`.
- The menu loop, input helpers, and formatted console output in `Game.cpp`.
- The input helper, free in-battle style switching menu, and formatted
  `std::cout` text in `BattleSystem.cpp`.
- Console labels returned by `ToString(...)` in `Models.h` if Godot stores its
  own display labels.

Search for `SANDBOX UI ONLY` to find temporary console code quickly.

## Developer Balance Tool

- Run `Poke-clone-esports.exe --balance 250` to execute mirrored automated
  battles and print a spec win-rate matrix.
- Mirrored battles swap turn order so first-attacker advantage does not make a
  spec look stronger than it is.
- Styles are randomized for each automated battle.
- Search for `DEV BALANCE TOOL ONLY` when this temporary workflow is replaced.

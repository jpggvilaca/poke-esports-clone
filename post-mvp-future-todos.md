# Post-MVP Future TODOs

This file is for ideas that should not distract from the MVP, but are worth keeping. The MVP should prove the core loop first: roster, battle, progression, rewards, and return-to-map flow.

## Core Post-MVP Systems

### Save / Load

- Persist trainer state, rating, money, roster, battle progress, defeated NPCs, trophies, and unlocked content.
- Save to `user://save.json` or an equivalent Godot save path.
- Decide whether C++ or Godot owns validation before implementing.

### Better Opponent AI

- Replace random affordable skill selection with scoring.
- Prioritize healing when low HP.
- Prefer stuns/silences when the player is about to use a strong skill.
- Prefer marks/debuffs when follow-up damage is available.
- Avoid wasting buffs, heals, or statuses already active.
- Eventually add personality profiles for AI opponents: aggressive, defensive, scaling, disruptor, opportunist.

### Player Recognition / Rival Memory

Inspired by the Shadow of Mordor-style nemesis fantasy: enemies should remember the human player and evolve based on encounters.

Possible behaviors:

- NPCs remember previous wins/losses against the player.
- An enemy that beats the player gains rating, traits, money, confidence, or new skills.
- Rivals comment on previous encounters.
- Repeatedly defeated enemies can adapt with counters to the player's most-used strategy.
- Strong enemies begin to recognize the player as a dangerous rising star after many wins.
- Major rivals can build reputation from defeating other NPCs too, not just the player.

Possible data:

```text
rival_id
times_fought_player
wins_against_player
losses_against_player
last_result
signature_skill
adaptation_tags
reputation_level
personal_dialogue_flags
```

### Roster And Skill Loadout Screen

- Dedicated roster management UI.
- Inspect all owned competitors.
- Compare stats, traits, skill levels, HP, mana, and rank.
- Equip and unequip active skills.
- Choose battle lineup visually instead of relying on map hotkeys.

### External Content Data

- Move skills, specs, traits, drills, and NPC battle configs out of hardcoded C++/GDScript.
- Use JSON, CSV, TOML, or Godot resources.
- Keep validation strict so broken IDs fail loudly.

## Game Feel And Presentation

### Sounds

- Button hover/click sounds.
- Skill use sounds by tone/type.
- Hit, miss, heal, buff, debuff, stun, silence, victory, defeat, reward, level-up sounds.
- Ambient map audio.
- Rival encounter stingers.

### Better UI

- Stronger battle HUD readability.
- More polished roster cards.
- Better skill tooltips with damage, chance, status, target, cooldown, and scaling.
- Clear turn order display.
- Better post-battle rewards panel.
- Visual distinction between specs, traits, and ranks.

### More Feedback

- Floating damage/heal numbers.
- Status icons with remaining turns.
- Cooldown and mana animations.
- Hit/miss animations.
- Screen shake or flash on important skills.
- Clear “why unavailable” feedback for skills.
- Rival memory callouts after repeat encounters.

## Multi-Genre Expansion

Right now the game data is MOBA-flavored. Post-MVP, the same player/esports framework could support multiple game genres.

### FPS Genre

Possible specs/roles:

- Entry Fragger
- Sniper
- Support
- Lurker
- IGL

Possible mechanics:

- Aim duels as damage skills.
- Flash/smoke as accuracy or action-denial effects.
- Economy rounds as between-battle modifiers.
- Clutch trait bonuses at low HP.
- Map-control status effects.

### RTS Genre

Possible specs/roles:

- Macro
- Micro
- Harass
- Tech
- Economy

Possible mechanics:

- Economy scaling over turns.
- Unit composition counters.
- Tech timing ultimate skills.
- Harass skills that reduce opponent mana/income.
- Defensive turtle buffs.

### Other Future Genres

- Fighting game: reads, combos, meter, guard breaks.
- Card game: deck consistency, draw, control, combo, fatigue.
- Sports sim: stamina, morale, formations, momentum.
- Battle royale: positioning, loot, zone pressure, survival instincts.

## Strategic Match Layers

### Pick And Ban / Draft Phase

- Add only after roster identity and counters are fun.
- Let players scout the opponent roster.
- Ban or counter-pick specs.
- Make draft matter without making battles too slow.

### Scouting And Intel

- Reveal enemy tendencies before battle.
- Show partial skill loadouts.
- Unlock better scouting with trainer progression.
- Hide some rival adaptations until discovered.

### Tournaments And Leagues

- Brackets.
- Seasons.
- Rival teams.
- Promotion/relegation.
- Special cups by genre or rating band.

## Progression Depth

### Trait Evolution

- Traits evolve from repeated behavior.
- Example: repeated clutch wins unlock a clutch trait.
- Example: repeated support play unlocks stronger team buffs.

### Skill Specialization

- Skills branch at level thresholds.
- Example: stun skill can become longer duration or higher damage, not both.
- Lets two players with the same spec feel different.

### Team Synergy

- Bonuses for lineup combinations.
- Genre-specific synergy.
- Rivalry or friendship between roster members.
- Team identity bonuses after repeated wins with the same lineup.

## Content And World

### Rival Dialogue

- Pre-battle callouts.
- Post-battle reactions.
- Recognition of player streaks, repeated wins, or embarrassing losses.
- Dialogue tied to rival memory.

### Venue Progression

- Small local matches.
- Regional leagues.
- Major halls.
- International events.
- World championship.

### NPC Growth

- NPCs gain XP, new skills, traits, or rating over time.
- Rivals can defeat each other offscreen.
- World state changes as the season progresses.

## Keep Out Of MVP

- Multi-genre support.
- Full nemesis/rival simulation.
- Draft mode.
- Complex save migration.
- Advanced AI personalities.
- Large external content pipeline.
- Multiplayer.

These are exciting, but the MVP should stay focused enough to finish.

# Balance Notes

## Running The Developer Report

Build the project, then run:

```text
Poke-clone-esports.exe --balance 250
```

This executes 500 mirrored battles per matrix cell: 250 with each spec acting
first and 250 with turn order reversed. Across five specs, that is 12,500
automated fights. Styles are randomized.

## Baseline: June 2, 2026

With advantage `1.25x` and disadvantage `0.75x`, counter specs won roughly
`94%` to `96%` of automated fights. Neutral and mirror matchups stayed close to
`50%`.

That is too strong for a fixed one-on-one matchup. The advantaged spec deals
more damage while receiving less damage for the entire fight. Unlike Pokemon,
the player cannot currently switch to another competitor after seeing the
matchup.

## Useful References

- The official Pokemon Brilliant Diamond and Shining Pearl battle guide shows
  clear effectiveness labels and large type modifiers, while also teaching
  players to switch Pokemon to improve a bad matchup:
  https://diamondpearl.pokemon.com/en-us/trainersguide/fundamentals/battling/
- The official Final Fantasy XIV Black Mage guide demonstrates a readable
  resource tradeoff: Astral Fire increases fire potency while also increasing
  MP cost, while Umbral Ice reduces cost and recovers MP:
  https://na.finalfantasyxiv.com/jobguide/blackmage/
- The official Final Fantasy XIV adjustments guide shows the tuning process in
  practice: potency, MP cost, HP, and timing values are adjusted based on
  observed performance and win rates:
  https://na.finalfantasyxiv.com/jobguide/adjustments/

## Next Tuning Hypothesis

Test softer spec modifiers before changing other values. A reasonable first
experiment is advantage `1.10x` and disadvantage `0.90x`, then compare the
counter-spec win rates with this baseline.

Keep skill power, focus costs, accuracy, and HP unchanged during that experiment
so the effect of the spec modifiers remains easy to understand.


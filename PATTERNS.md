# NarrativeEngine Plugin — Pattern Language

The narrative simulation language. It completes [[Narrative Owns World
Meaning]] in the root `PATTERN_LANGUAGE.md` and defers there for form and
process. Read top-down to grow the simulation; read bottom-up to trace why a
line was worth saying.

The language serves [[Narrative Puzzle]] — dialog as a puzzle over simulated
state — and [[A Game Worth Decades]]: this plugin is the part of Dungeon
Break meant to deepen for decades the way Dwarf Fortress deepens — by asking
what stories the sim cannot yet tell, and building until it can.

**Context held constant** (from the code as it stands): narrative state is an
N-dimensional space whose axes are `UNarrativeBasisVector` assets; entities
are particles (`FNarrativeEntityInstance`) with Position, Mass, Telos, and
Motive, integrated by fixed-step Verlet in `UNarrativeSubsystem`; actions are
forces (`UNarrativeActionDef`); dialog defs are located phrases with
gravitational reach (`UNarrativeDialogDef::Location`, `SpaceSigma`). The
plugin is game-agnostic; the game module (`Narrative`) derives meaning from
it, never the reverse.

```
THE WORK
1. THE LEGENDARY LINE *
   2. THE SIM DOES NOT KNOW IT IS WATCHED **
   3. THE LADDER OF LEGEND

THE SPACE
4. FEW AXES, RICH AXES **
   5. EVERYTHING IS A FORCE **
      6. CANON GAINS MASS

THE ENCOUNTER
7. THE SEAM IS PLAYABLE *
   8. SAME TABLE, SAME RULES *
   9. MUNDANE BETWEEN MIRACLES *
      10. NO DEAD CARDS *
      11. SILENCE IS A CARD

THE LINE
12. EVERY LINE RETURNS
    13. LET REGISTERS COLLIDE *
    14. KEEP THE WEIRD *
       15. A LINE THAT COULD NOT BE OTHERWISE *
```

---

## 1. THE LEGENDARY LINE *

Within [[Narrative Puzzle]] and [[A Game Worth Decades]], the sim can grow in
any direction, and every direction looks like progress.

◆◆◆

**A simulation grown feature-by-feature becomes a physics demo: correct,
busy, and mute. A story system grown scene-by-scene becomes a script:
expressive, and dead on the second playthrough.**

Tarn Adams grows Dwarf Fortress by imagining the story first — the werebeast
siege, the artifact sock — then building simulation until the story can
happen without him. The unit of planning is not a system but an utterance.
Keep a living list of legendary lines; it currently reads:

- *That was fucking magical*
- *A heart wants what it wants*
- *You're a failure to your ancestors*
- *Gender got hands*

Each line is a center the plugin organizes itself around, the way rooms
arrange around a hearth. A feature that serves no line is ornament; a line no
feature can reach is a lie. Selecting a legendary line in play applies
massive narrative force — it canonizes, cementing the decision in the
distribution of information within the sim (see [[Canon Gains Mass]]).

◆◆◆

**Therefore: plan backwards from impossible lines. Before building a system,
name the legendary line it exists to make possible. Before adding a line to
the list, sketch the sim state that must be true for it to land convincingly
— who witnessed what, which axes are loaded, what history exists. Build until
that sketch is real; stop when it is.**

*Example:* "You're a failure to your ancestors" demands lineage records, an
inherited `Telos`, mass as "inherited will," and a judge who knows both. One
line specs three systems.

Completed by [[The Ladder of Legend]], [[Canon Gains Mass]], and — writ
small — [[A Line That Could Not Be Otherwise]].

---

## 2. THE SIM DOES NOT KNOW IT IS WATCHED **

Within [[The Legendary Line]], completing [[Narrative Owns World Meaning]]:
the plugin owns abstract state simulation and stays game-agnostic.

◆◆◆

**State that exists only when the player looks at it can never surprise the
player. But a sim entangled with gameplay actors cannot run without them —
and dies in off-screen regions, time skips, and headless tests.**

`UNarrativeSubsystem` already ticks every `FNarrativeEntityInstance`
"regardless of whether or not they're being observed" — fixed-step Verlet, no
LOD on meaning. `UNarrativeEntityComponent` is a proxy: the actor registers
against an entity; it does not own one. Legendary lines are earned
off-screen: "A heart wants what it wants" only lands if the heart wanted it
before you asked. The first `if (PlayerIsPresent)` branch inside this plugin
turns the world into scenery.

◆◆◆

**Therefore: no observer-privileged code path in this plugin. Gameplay reads
the sim through components and delegates, may apply forces, and never holds
sim state. If a feature needs the sim to behave differently while watched,
the feature belongs in the game module, not here.**

*Example:* `UNarrativeEntityComponent::GetLocation()` reads;
`QueuedImpulseForces` writes; nothing actor-side stores a position.

Completed by [[Everything Is a Force]], [[Same Table, Same Rules]].

---

## 3. THE LADDER OF LEGEND

Within [[The Legendary Line]], information must live somewhere between a
millisecond impulse and a myth that outlasts its subject.

◆◆◆

**A sim with only instantaneous forces has no memory — nothing is ever
*about* anything that happened. A sim that records everything at full
resolution drowns in its own history and cannot say what mattered.**

Give information distinct rungs, each a real structure made of the rung
below, each within sight of its neighbors:

**moment** (an impulse this tick) → **memory** (a moment retained by a
witness) → **rumor** (a memory in transit between entities) → **legend** (a
rumor held so widely it no longer needs its witnesses) → **myth** (a legend
that outlived its subject and became a direction of the space itself — a
candidate `UNarrativeBasisVector`).

Each promotion should cost roughly an order of magnitude more reach and
lifetime than the last, so the ladder is climbable but felt. Canonization is
the player's deliberate exception: a legendary card jumps rungs at once, and
the ladder exists so that jump is expensive and audible.

◆◆◆

**Therefore: every new system names the rungs it reads and writes, and may
only write to rungs adjacent to what it reads — except canonization. The top
rung is the basis itself: when a legend has outlived its proper nouns,
consider promoting it into a basis vector.**

*Example:* "Gender got hands" is a myth-rung line — slang exists only once a
legend has been retold enough times to lose its names.

Completed by [[Canon Gains Mass]], [[Every Line Returns]].

---

## 4. FEW AXES, RICH AXES **

The space itself: everything in this plugin lives in coordinates over
`UNarrativeBasisVector` assets.

◆◆◆

**Every concept the game needs tempts a new axis. But each axis dilutes
every distance, dot product, and sigma reach already tuned — a hundred thin
axes make all meanings equidistant, and the space goes flat.**

The subsystem header already commands it: "It can't matter how many basis
vectors are defined, but quality should be prioritized over quantity." An
axis is not a stat. `UNarrativeBasisVector` carries a title, an icon,
sub-components, and a grammar — Verb, Subject, Adjectives — a world of
meaning that lines are generated *from*. The calm of the space comes from
every axis being irreplaceable.

◆◆◆

**Therefore: before adding a basis vector, enrich one — add sub-components,
deepen the grammar, widen what the axis can mean. Add a new axis only when
two legendary lines demand directions the existing axes cannot distinguish.
When an axis stops appearing in any line, fold it into a neighbor.**

*Example:* "That was fucking magical" does not need a Wonder axis if an
existing axis's `Adjectives` can carry awe.

Completed by [[Everything Is a Force]], [[No Dead Cards]].

---

## 5. EVERYTHING IS A FORCE **

Within [[The Sim Does Not Know It Is Watched]], gameplay and design will
constantly ask the sim yes/no questions.

◆◆◆

**Booleans are how narrative sims die. Each flag is a cliff in state space —
unearnable by degrees, unreversible by degrees, invisible to the physics —
and ten flags make a script with extra steps.**

The plugin's vocabulary is already continuous: `UNarrativeActionDef` is a
Force map; dialog reach falls off over `SpaceSigma`; threat "is a scalar that
scales negatively into affection"; Mass, Drift, and Damping shape how meaning
moves. In the encounter mockup, Alt toggles the verb between interact and
attack, but the *default* hostility is derived from narrative state — a
read, not a switch. "Is she hostile?" should be answered the way "is it
uphill?" is: by sampling the field at a point.

◆◆◆

**Therefore: new narrative state must be a position, a force, or a parameter
of motion — never a flag. When gameplay needs a yes/no, derive it as a
threshold-read over the fields, and keep the threshold in the game module,
not the plugin.**

*Example:* hostility belongs in `Narrative`, derived through
`UNarrativeEntityComponent` (cf. `GetAlignmentTo`) — sampled, never stored.

Completed by [[Canon Gains Mass]], [[The Seam Is Playable]].

---

## 6. CANON GAINS MASS

Within [[Everything Is a Force]] and [[The Ladder of Legend]]: a legendary
card applies massive narrative force, and something must make it stick.

◆◆◆

**A force with nothing to push against is drama that evaporates — next tick,
drift and damping erase the vow. But state that is simply written — pinned,
locked, flagged — breaks [[Everything Is a Force]] and can never be
tragically undone.**

Mass already means this: "conviction, or potential impact on peoples'
inherited will." A heavy entity resists external force and carries more
inertia along its telos. Canonization needs no new mechanism, only an edge
in the physics: when accumulated force crosses a threshold, convert the
overflow into mass instead of velocity. The line between rumor and canon
becomes a real, thick region — approached gradually, crossed decisively,
felt forever after. Canon can still be moved, but only by something with
more conviction than what made it. That is the tragedy budget. Whether mass
stays scalar or becomes per-axis is the first question an implementation
must answer; the pattern holds either way.

◆◆◆

**Therefore: canonize by mass transfer at a threshold, never by locking
state. A legendary line's force should mostly become mass — in the speaker,
the witnesses, and the fact itself — so the sim remembers by becoming harder
to move, not impossible to move.**

*Example:* playing *Vow* ("I will stop the Dungeon Breaks!") should raise the
vower's Mass and bend their `Telos` toward the vow; breaking it later must
cost a counterforce felt by every relationship touching that mass.

Completed by [[Every Line Returns]].

---

## 7. THE SEAM IS PLAYABLE *

Within [[Everything Is a Force]]: the encounter mockup binds Interact and
Attack to the same mouse on the same target, Alt toggling between them.

◆◆◆

**If dialog and combat are separate modes with a hard cut between them, the
most dramatic moment in RPG conversation — the instant talk becomes violence
— belongs to neither system, and plays as a scene change instead of a
decision.**

Cards are already both speech and physics: they "can be mechanical or apply
impulse forces to perturb the narrative sim." A *Threaten* is a combat act
delivered as grammar; a sword drawn mid-sentence is a dialog act delivered
as steel. Let the two systems reach into each other far enough that some
acts are irreducibly ambiguous — the ambiguity is the good part. Combat
outcomes apply narrative impulses; narrative state prices combat.

◆◆◆

**Therefore: build talk and violence as one action pipeline with two
renderings. Every card declares its mechanical payload and its narrative
impulse in the same def; every combat verb declares the narrative impulse it
applies. The hostility seam is a derived read that either side can push
across mid-encounter, in both directions.**

*Example:* `UNarrativeActionDef::Force` is the shared payload shape — a
*Threaten* card and an opening attack can share the same def.

Completed by [[Same Table, Same Rules]], [[Silence Is a Card]].

---

## 8. SAME TABLE, SAME RULES *

Within [[The Sim Does Not Know It Is Watched]], the player must join
encounters, not host them.

◆◆◆

**If the player's dialog machinery is bespoke — special hand, special
options, special consequences — every NPC becomes a vending machine, and no
legendary line can convincingly be said *to* the player.**

`UNarrativeDialogDef` is already written as "one possible incoming thought
that's simulated for a character": the same defs that fill the player's hand
are the thoughts NPCs choose from by proximity and sigma. Hold the symmetry
at every table — both sides draw from position, both spend conviction, both
can canonize. The asymmetries that make it a game — Effort, local pause,
rerolling by Focus or Think — are privileges of *deliberation*, not a
different physics. Asymmetric globally, symmetric at every table.

◆◆◆

**Therefore: NPCs speak through the same card pipeline the player does —
same defs, same gravitational draw, same impulses on selection. Player-only
mechanics may govern choosing (time, rerolls, information) but never
outcomes. The test: any legendary line the player can say, some NPC could in
principle say — including to the player.**

*Example:* "You're a failure to your ancestors" is best delivered *by* an
elder *to* the player — which only works if NPC speech runs the full
pipeline.

Completed by [[No Dead Cards]], [[Every Line Returns]].

---

## 9. MUNDANE BETWEEN MIRACLES *

Within [[The Legendary Line]]: a language of only legendary lines is a
language of none.

◆◆◆

**Tune the generator toward the spectacular and every conversation escalates
to myth; players acclimate within an hour and the legendary rung reads as
flavor text. Tune it flat and encounters are wallpaper.**

The mockup's hand shows the shape: *See Wares* sits beside *Vow*, and two
ordinary cards make the third card's thunder audible. The same beat repeats
at every scale — within a hand (most cards cheap, one dear), within an
encounter (Focus gains Effort, Think spends it: breathing in and out),
within a session (shops and small talk between canonizations). The
alternation carries the drama, not the peaks.

◆◆◆

**Therefore: enforce rarity structurally, not by dice. Legendary cards must
be priced in Effort and state the player cannot pay often, and the
generator's distribution must keep mundane cards the overwhelming mode. When
tuning, count the beats between miracles — widen the interval before you
brighten the miracle.**

*Example:* in the V5 mockup, *Vow* costs ⚡5 against *Boast*'s and *See
Wares*'s ⚡2 — the hand itself alternates cheap, cheap, dear.

Completed by [[No Dead Cards]], [[Silence Is a Card]].

---

## 10. NO DEAD CARDS *

Within [[Mundane Between Miracles]] and [[Few Axes, Rich Axes]], the hand is
the player's entire view into the space.

◆◆◆

**A generator fills hands easily; a hand padded with unreachable, redundant,
or irrelevant options teaches the player that cards are noise. They stop
reading — and dialog dies as a puzzle.**

The machinery for relevance exists: a `UNarrativeDialogDef` has a `Location`
and a `SpaceSigma`, and a card should appear only where its reach genuinely
overlaps the present state of both parties. The hand is small — the mockup
deals four — so every slot must hold a whole option: reachable from here,
pointed somewhere its neighbors are not, worth what it costs. Rerolling by
Focus and Think exists so the *player* sculpts the hand; it must never
become the generator's excuse for padding it.

◆◆◆

**Therefore: every dealt card passes three checks — reachable (within sigma
of present state), distinct (its impulse direction differs meaningfully from
every other card in the hand), and priced (its Effort reflects its force).
If the generator cannot fill the hand under these checks, deal a short
hand.**

*Example:* *See Wares* earns its slot with a merchant because commerce
genuinely neighbors the state; the identical card in a graveyard is a dead
card.

Completed by [[Silence Is a Card]], [[A Line That Could Not Be Otherwise]].

---

## 11. SILENCE IS A CARD

Within [[No Dead Cards]]: the deepest option in any exchange.

◆◆◆

**A dialog system that always demands a line makes every silence a UI
failure, and fills the world's quiet with barks — until nothing said means
anything.**

Walking away, saying nothing, letting an elder's judgment hang unanswered —
these are moves, and the physics already knows how to resolve them: silence
applies no counterforce, so the field's own drift and damping act
uncontested. Yielding is a choice with consequences, not an absence of one.
The same restraint applies to the interface: the sim should not explain
itself. Unnarrated state is where player projection lives — Dwarf Fortress's
best stories are half inference — and an inspector panel that reveals every
motive drains the dark that makes lines land.

◆◆◆

**Therefore: the null move is always in the hand, costs nothing, and is
never a menu-cancel — choosing silence commits the exchange and lets
uncontested drift resolve it. And surface less than the sim knows: state
reaches the player through lines and consequences, never through gauges of
motive.**

*Example:* against "You're a failure to your ancestors," silence — accepting
the field's pull — should be the most eloquent legal reply.

Completed by [[A Line That Could Not Be Otherwise]].

---

## 12. EVERY LINE RETURNS

Within [[Canon Gains Mass]] and [[The Ladder of Legend]]: canon that never
resurfaces was never canon.

◆◆◆

**Replay a canonized moment verbatim and it is a cutscene reference —
recognizable and dead. Never let it recur and canonization spent its massive
force on a stat change nobody witnesses.**

A legend climbs the ladder by retelling, and retelling transforms: proper
nouns wear off, verbs strengthen, credit drifts toward whoever the reteller
admires or resents. Each echo is a new dialog-def-shaped thought spawned
near the canon's location in the space but voiced from the reteller's
position — the same family of shape, never the same member. "Gender got
hands" reads as the terminal echo of some solemn canon whose original words
nobody remembers, and that is exactly the effect to build.

◆◆◆

**Therefore: when a fact canonizes, spawn its echoes — derived dialog defs
owned by witnesses, positioned near the canonical `Location` but displaced
toward each witness's own state, strengthened or decayed per retelling. An
echo never reproduces the original `Phrase`; regenerate it through the
teller's grammar (the basis Verb, Subject, Adjectives).**

*Example:* the *Vow*'s "I will stop the Dungeon Breaks!" returns weeks later
as a merchant's "You're the one who swore about the Breaks, no?" — same
location in the space, different speaker position.

Completed by [[Let Registers Collide]], [[Keep the Weird]].

---

## 13. LET REGISTERS COLLIDE *

Within [[Every Line Returns]], and within the legendary list itself.

◆◆◆

**A generator tuned to one register — all high solemnity, or all quip —
produces tonal wallpaper. But registers mixed at random read as bugs, and
reviewers will "fix" them into one or the other.**

Look at the list: "You're a failure to your ancestors" beside "Gender got
hands." The list is the register spec, heterogeneous on purpose. "That was
fucking magical" is itself the collision — transcendence in gutter grammar —
and the awe is only believable *because* of the profanity. Collision must be
earned by the sim, not sprinkled on: register should follow the speaker's
position and the line's rung on the ladder, so myth-rung slang and
moment-rung solemnity each trace to a source. Opposites sharpen each other
exactly when each is true to where it came from.

◆◆◆

**Therefore: give registers coordinates, not a global tone slider — a line's
register derives from its speaker's position and its rung. Never tone-police
generated output in review: if a collision traces to state, it ships; if it
does not, fix the state, not the words.**

*Example:* the four-line legendary list is the register spec; keep it
heterogeneous when extending it.

Completed by [[Keep the Weird]].

---

## 14. KEEP THE WEIRD *

Within [[Let Registers Collide]]: the review gate for generated content.

◆◆◆

**Polish is the natural enemy of evidence. Every pass that smooths generated
dialog toward house style erases the trace of simulation — until players
cannot tell emergent from authored, and stop believing anything emerged at
all.**

Dwarf Fortress's cat-vomit deaths and artifact socks are load-bearing: the
roughness is *how* players know the sim is real. A line slightly off — odd
adjective order, a metaphor stretched by grammar recombination — is a
fingerprint. Perfectly smooth output reads as authored, and authored reads
as finite. The roughness must be real, though: sim-sourced oddity, never RNG
spice or injected typos. Fake roughness is smoother than smoothness.

◆◆◆

**Therefore: review generated lines for traceability, never for style. A
weird line that traces to state ships; a bland line that traces to nothing
is the bug. Spend polish on the grammar's bones — the basis Verb, Subject,
Adjectives — so the system produces its own kind of good sentence, then let
it.**

*Example:* "Gender got hands" survives review because it traces. That is the
whole test.

Completed by [[A Line That Could Not Be Otherwise]].

---

## 15. A LINE THAT COULD NOT BE OTHERWISE *

The smallest whole in the language — one card, one `Phrase`, one Label.
Every pattern above converges here.

◆◆◆

**A generated line that could appear in any game, from any speaker, at any
moment, is filler no matter how well it reads — and each such line spends
the credibility of every real line around it.**

A good line's shape is definite: short, concrete, spoken from a position,
assembled from the simplest parts — a Verb, a Subject, Adjectives — into
something with a center of its own. The test runs backwards: from the line
alone, a designer should be able to reconstruct the state — which entity,
which axis was loaded, which rung it came from. "A heart wants what it
wants" lands only when the sim can show you the heart and the want. This is
[[The Legendary Line]] writ small: pattern 1 works forward from a line to
the systems it demands; this pattern works backward from a line to the state
that earned it. The language is a loop.

◆◆◆

**Therefore: every line — authored def or generated echo — must be
checkable: name the entity, the axis, and the rung that produced it, or cut
it. When a line fails the check, the fix is upstream — enrich the axis, move
the def's `Location`, or admit the needed system does not exist yet and add
the line to the legendary list as its spec.**

*Example:* `UNarrativeDialogDef` is already the checkable unit — a `Phrase`,
where it lives (`Location`), and how far it reaches (`SpaceSigma`).

---

### A note on the form

Each pattern above is shaped on one of the fifteen properties of living
structure from *The Nature of Order* — one property per pattern, fifteen for
fifteen. The mapping is deliberate, but every pattern must stand without it;
if one only makes sense through its property, revise the pattern.

| # | Pattern | Property |
|---|---------|----------|
| 1 | The Legendary Line | Strong Centers |
| 2 | The Sim Does Not Know It Is Watched | Not-Separateness |
| 3 | The Ladder of Legend | Levels of Scale |
| 4 | Few Axes, Rich Axes | Simplicity and Inner Calm |
| 5 | Everything Is a Force | Gradients |
| 6 | Canon Gains Mass | Boundaries |
| 7 | The Seam Is Playable | Deep Interlock and Ambiguity |
| 8 | Same Table, Same Rules | Local Symmetries |
| 9 | Mundane Between Miracles | Alternating Repetition |
| 10 | No Dead Cards | Positive Space |
| 11 | Silence Is a Card | The Void |
| 12 | Every Line Returns | Echoes |
| 13 | Let Registers Collide | Contrast |
| 14 | Keep the Weird | Roughness |
| 15 | A Line That Could Not Be Otherwise | Good Shape |

Per [[The Language Precedes the Code]]: unmarked patterns (3, 6, 11, 12) are
hypotheses awaiting their first implementation — challenge them there.

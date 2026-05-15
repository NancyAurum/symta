# Why Symta uses ECS

*Design notes behind the `cls`, `dsm`, and IPS macros in
`src/cls.s`.  Companion reading for `runtime/am.h` (the adaptive
map that backs every component table) and the runtime-level GC
story in `runtime/gc.c`.*

---

## 1.  The thesis in one paragraph

For programs whose **state changes shape over time** — games,
compilers, simulations, planners — an Entity-Component-System
(ECS) layout is the most flexible, parallelisable, and
cache-friendly representation we have.  It collapses to a tiny
restriction of the relational model.  It eliminates the standard
object-oriented failure modes (god objects, deep hierarchies, the
diamond problem, the visitor-pattern dance) without giving up any
expressiveness.  And once the language has a GC, it costs almost
nothing extra at the runtime layer.  Symta is built on it; this
note explains why.

---

## 2.  A tour through entity organisations

Every program above toy-size eventually arrives at the same
question: *how do I lay out the things my program is about?*  The
answer evolves through five recognisable tiers, each solving the
previous tier's problems and introducing its own.  Walking the
ladder is the clearest way to see what ECS actually buys you.

### Tier 0 — Globals

```c
float rabbit_xyz[3];
float plant0_xyz[3];
float plant1_xyz[3];

void update(void) {
  update_plant(plant0_xyz);
  update_plant(plant1_xyz);
  update_rabbit(rabbit_xyz);
}
```

Fast.  Trivial.  Works.  This is how MS-DOS drove floppy drives,
how Atari 2600 cartridges drove the playfield, and how anyone
sketching a prototype starts.  It breaks the moment the entity
count stops being statically known.

### Tier 1 — Disjoint structs

```c
struct plant_t  { ... };
struct animal_t { ... };

plant_t  *plants;
animal_t *animals;
```

Now we can dynamically allocate.  The cost: any time two kinds of
entity need to interact, we either duplicate code across
`update_plants` / `update_animals`, or introduce an `event_t`
union that translates between them.  Sorting plants and animals
together by camera distance requires a temporary super-structure.
The simplicity of Tier 0 is gone.

### Tier 2 — One fat struct with a type tag

```c
struct entity_t {
  etype_t   type;
  float     xyz[3];
  /* everything any kind of entity might ever need */
  int       visual;
  float     speed;
  ai_t      agency;
  limb_t   *limbs;
  branch_t *branches;
  ...
};
```

Now any entity can interact with any other; iteration is a single
loop with a `switch (e->type)`.  The cost is memory:  every event
carries an `ai_t` it will never use.  This was the late-80s /
mid-90s shape, and it's still the right answer for small programs
where memory is cheap and the entity count is bounded.

### Tier 3 — Tagged union state

```c
struct entity_t {
  float    xyz[3];
  state_t *state;          /* shared, COW per modification */
};

struct state_t {
  etype_t type;
  union {
    plant_t  plant;
    animal_t animal;
    event_t  event;
  } data;
};
```

We've squeezed the memory bloat by hoisting the variant data
behind a pointer, and we get a free flyweight pattern (all
identical plants share one `state_t` until something modifies
one).  This is the 2000s shape: most game engines that grew out
of that decade are still recognisably Tier 3 under the hood.

Two pains remain:

1.  **Implicit components.** Logic like `IS_VISUAL(e)` is really
    asking "does this entity have a visual component?", but the
    component list is encoded in the type tag.  Adding "now this
    one specific tree is also a clickable button" requires
    inventing a new type.
2.  **No identity.** Entities are referred to by pointer, so any
    metadata you want to attach later (expiration time, network
    id, debug name) lives in a side hash-table.  That hash-table
    is, in a small way, the beginning of a database — except you
    only have one.

### Tier 4 — Components list on the entity

```c
struct entity_t {
  int  *components;   /* the list of components this entity has */
};

void run_phase(entity_t *es, int phase) {
  for (int i = 0; i < alen(es); i++)
    for (int j = 0; j < alen(es[i].components); j++)
      phase_fn[ es[i].components[j] ][phase] (&es[i]);
}
```

Made famous by the Dungeon Siege GDC talk in 2002, popularised
through the 2010s.  Components are now first-class: adding
"clickable" to a tree means appending a tag to one list.  This is
the most common shape in modern engines.

The remaining wart is that the entity *owns* its component list.
Iteration walks the entity array, which is still AoS in spirit —
the per-component data is hashed off elsewhere but the dispatch
loop is entity-major.  Filtering for "all entities with both
`renderable` and `clickable`" still touches every entity.

### Tier 5 — Pure-id ECS

```c
typedef int entity_t;          /* yes, just an int */
table_t  **component_tables;   /* one per component */

void run_phase(int phase) {
  for (int t = 0; t < alen(component_tables); t++)
    if (component_tables[t].phase[phase])
      foreach_key(eid, component_tables[t])
        component_tables[t].phase[phase](eid);
}
```

An entity is now nothing but a key.  All data lives in
component-indexed tables.  Iteration is **component-major**:
"every entity with `velocity` advances by `velocity`" reads
through one cache-friendly column.  Adding a component is one
table insert; querying "everyone with `renderable` AND
`clickable`" is one set intersection.

This is the shape Looking Glass arrived at in the mid-90s for
Thief and System Shock, where the design demanded that any
object be capable of becoming any other (a torch could become a
weapon, a guard could become a corpse and the corpse could remain
a discoverable object).  It is the shape Symta's `cls` macro
compiles to.

---

## 3.  Why each step matters: AoS, SoA, and what "fast" means

The common dismissal of ECS — "it's just a buzzword" — almost
always misses that **the memory layout changes**.  Compare:

```c
/* AoS — array of structs (Tier 2-4) */
struct agent { int guid; int type; bool ally; bool herbivore; ... };
struct agent agents[N];
```

Counting all herbivores reads every byte of every struct.  Even
with a bit-packed struct, the CPU still loads the full cache line
per entity.

```c
/* SoA — struct of arrays (Tier 5) */
uint64_t herbivore_bitmap[N / 64];
```

Counting all herbivores reads `N/64` 64-bit words.  An order of
magnitude less memory; an order of magnitude less cache miss; and
SIMD popcounts the lot in a few instructions.

The win compounds: most game state is boolean tags and small
enums, which compress trivially in column-major form.  When
adjacent rows of a column share a value (and they often do — most
entities aren't carnivores, most tiles aren't damaged, most
particles haven't expired), a one-dimensional sparse-bitmap tree
brings storage *below one bit per entity*.  ECS doesn't
*introduce* this efficiency; it makes it natural.

That same column layout is what GPUs need.  Compute shaders are
SoA by force of hardware: passing AoS data to a kernel requires a
transpose pass.  The ECS choice and the GPU choice are the same
choice.

---

## 4.  ECS is just restricted relational algebra

The SpacetimeDB folks observed
([databases-and-data-oriented-design](https://spacetimedb.com/blog/databases-and-data-oriented-design))
that ECS is the relational model with four restrictions:

1.  There is exactly one entity table, with one unique id column.
2.  All other tables foreign-key on that entity id.
3.  "Systems" (queries) run on every matching entity, every frame.
4.  Queries are inner joins on the entity id.  No ordering, no
    range queries, no joins on other columns.

Drop those four restrictions and you have SQL.  Add them back and
you have a system that fits in a video-game frame budget.  This
is why the old aphorism — *"any sufficiently complicated Lisp
program contains an ad-hoc, informally-specified, bug-ridden,
slow implementation of Prolog"* — has so much teeth in this
domain.  Game logic is relational.  Pretending it isn't, by
encoding it in object hierarchies, is the source of most of the
spaghetti.

Symta makes the relational shape first-class via the `cls` and
`dsm` macros: declarations look like type definitions, but the
back-end is always tables-of-rows.

---

## 5.  Relations as dynamic component tables

Plain components handle the unary case: "Alice is a programmer."
The binary case — "Alice owes Bob 1000 euros" — needs more.

The clean encoding is to **materialise the relation as a
component table whose key encodes the predicate and one
argument**:

```
ComponentTables["[Verb]_[Obj]"][Subject] = optional value

ComponentTables["eats_apples"][Rabbit.id]      = 2
ComponentTables["owes_eur_[Bob.id]"][Alice.id] = 1000
```

Queries fall out:

- *What does Alice eat?* — scan tables prefixed `eats_`, look up
  `Alice.id` in each.
- *Who eats apples?* — list keys of `eats_apples`.
- *Who owes Bob euros?* — list keys of `owes_eur_[Bob.id]`.
- *Bidirectional inference* — every fact lives in both a forward
  and a backward table, so backward chaining is symmetric with
  forward chaining and costs the same.

The catch: a working program might create tens of millions of
these tables, most of them holding a handful of entries.  The
naïve `unordered_map<int,int>` per relation is fatal.  This is
precisely what `runtime/am.h`, Symta's *adaptive map*, exists to
solve: an empty table is a tagged immediate, a 1-entry table is
two inlined slots, a small dense table is a bitmap, and only
medium-and-up tables pay for the hash machinery.  The cost curve
is calibrated for the relations workload.

Example 27 in `examples/27-relational.s` is a 100-line working
demonstration: forward and backward chaining, transitive closure,
direct and inverse queries, all on top of two adaptive maps.

---

## 6.  Memory management: ECS meets a generational GC

This is the question that derails most ECS implementations, and
it's worth being explicit about Symta's answer.

**The problem.**  In a non-GC'd ECS, freeing entity 42 means
walking every component table and erasing the row keyed on 42.
The entity holds a set of "components I have" to make that walk
finite.  Two failure modes follow:

- Every read goes through that membership set (cache miss).
- A `free(42)` racing with a queued message addressed to 42 is a
  use-after-free; in practice, everything ends up on one thread.

We threw away most of ECS's parallelism to get manual lifetime
management.  Bad trade.

**The solution: invert who owns the memory.**  Each component
table owns its own rows.  Nothing in the entity points back at
its components.  Freeing an entity means *"systems, you may now
drop rows keyed on this id."*  Looking Glass arrived at this for
Thief and System Shock; it's the only design that survives
contact with parallelism.

That leaves the question of *what* tells the systems an id is
dead.  In Symta, the answer is the generational GC.

**The protocol.**  Cheney semi-space, generations stacked over
the heap:

1.  On collection, GC walks the live set as usual — entity ids
    reachable from roots are alive.
2.  The set of *unreachable* ids in the collected generation is
    materialised as a sorted compressed integer set.  For
    archetype ranges (entities allocated in contiguous id blocks
    — particles, voxels, tilemap cells) the set degenerates to a
    range, which costs O(1) to represent and O(range-len) to
    iterate.
3.  This unreachable set is broadcast to every component table.
    Each table runs its own finalisation phase, removing rows
    keyed on dead ids.  This is just another system, scheduled
    after the regular phase pipeline.

The protocol has three properties worth highlighting:

- **Incremental.** Component-table finalisation is per-table and
  parallelisable.  Slow tables don't block fast ones.
- **Race-free.**  Until a table has acknowledged the dead set, it
  is still safe to send a message to that id; the message will
  simply have nothing to apply to in tables that have already
  cleared.  Cross-system "did this id exist last frame" queries
  observe a coherent snapshot per phase boundary.
- **Cheap for hot churn.**  Particle systems that allocate and
  free thousands of ids per frame produce a contiguous dead range
  on each minor collection, not a list.

The implementation lives partly in `runtime/gc.c` (producing the
dead set) and partly in `src/cls.s` (the per-system finalisation
phase generated by the `cls` macro).

---

## 7.  Parallelism and determinism

A pleasant property of the systems-own-their-rows arrangement is
that systems in the same phase can be split across threads
trivially.  Each system reads its own component tables and writes
to others through a message buffer; there are no shared mutable
objects.

A less pleasant property: iteration order in a hash-keyed table
depends on the keys present.  In a lock-step networked game,
client A and client B will produce different iteration orders if
their entity id allocations diverge — even if the divergence is
in entities client A doesn't simulate (camera, visualisers).  Two
units trying to move into the same tile then resolve differently
on each machine.  Five workable fixes, in increasing order of
robustness:

1.  **Reserve an id range for shared entities.**  Fastest;
    fragile to programmer discipline.
2.  **Insertion-ordered tables** for the systems where order
    matters.  An indirection layer, modest cost.
3.  **Sort by id at iteration time** in deterministic systems.
    O(n log n) per frame on the affected tables.
4.  **Hybrid component storage** — let order-sensitive components
    live in a B-tree instead of a hash.  Memory cost.
5.  **Make resolution order-independent.**  Movement system
    proposes; a resolution system detects two-into-one collisions
    and adjudicates them by, e.g., momentum.  Most robust,
    because it acknowledges the underlying physics.

Symta's stdlib does (2) by default for `dsm` (deterministic
system maps) and exposes (1) for fast paths.  (5) is a design
choice that lives in user code, not the framework.

---

## 8.  How Symta wires this up

The high-level API in `src/cls.s` is three macros:

| Macro | Role |
|------|------|
| `cls`  | Declares a component class.  Generates the component table, the accessor methods, the finalisation hook, and an archetype factory. |
| `dsm`  | "Deterministic system map" — the iteration primitive that runs a system over the entities matching a component query. |
| `IPS`  | "Indexed property store" — the per-phase scheduler.  Maps systems → phases, runs them in order, takes parallelism hints. |

Underneath, every component table is an `am_t` (adaptive map,
`runtime/am.h`) parameterised on key type.  The adaptive-map
choice is what makes the millions-of-tiny-relations workload
viable: empty tables cost a tagged immediate, tables with one
entry cost two cells, dense tables compress to bitmaps, and the
hash machinery only kicks in when the table actually grows.

The runtime-level reader for this design lives in
[`../architecture.md`](../architecture.md); the adaptive-map
details are in `../runtime/am.h`.

---

## 9.  When *not* to use ECS

ECS trades simplicity and static safety for flexibility.  Some
real costs:

- **Hard to type-check.** An entity is just an id; "this entity
  has a `position`" is a runtime invariant, not a compile-time
  one.  Symta's gradual type system (see `TODO.md` Milestone 2)
  is partly motivated by closing this gap.
- **Hard to debug without good tooling.**  "What does entity 542
  consist of right now" is a multi-table query, not a struct
  print.  Build the inspector early.
- **Wrong for small programs.** A 200-line script does not need a
  database.  Tiers 0 through 2 are still the right answer for
  small, contained, slowly-evolving code.

The right rule of thumb: if your program's state will outlive the
program's design, use ECS.  If you'll throw the program away in a
week, don't.

---

## 10.  Further reading

- *Dungeon Siege* GDC 2002 talk — the original components-list
  presentation that started the modern wave.
- A. Mertens, *Building games in ECS with entity relationships* —
  the relations-as-dynamic-tables pattern in Flecs.
- *SpacetimeDB: Databases and Data-Oriented Design* — ECS as a
  restriction of the relational model.
- R. Fabian, *Data-Oriented Design Book* — the layout-first
  perspective on the same problem.
- Looking Glass Studios *Dark Engine* documentation — the
  Act/React (Stimulus/Receptron) system, an early production ECS.

---

## Glossary

- **Archetype** — a fixed combination of component types,
  typically used to spawn many entities in a contiguous id range.
- **Component** — a column.  A single property attached to (some
  of) the entities.
- **Component table** — the column storage for one component.
  Keyed by entity id; sparse.
- **Entity** — a row, identified by a unique integer id.  Has no
  data of its own.
- **Phase** — a stage in the per-frame pipeline.  Conventional
  order: input → react → update → output → visualise → reclaim.
- **Relation** — a binary (or higher-arity) component, materialised
  by encoding one argument into the table key.
- **System** — a function over entities that share a component
  signature.  Runs once per matching entity per phase.
- **Tag** — a component with no value.  Costs one bit in the
  bitmap mode.

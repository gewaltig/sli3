# Hot operator catalog

A curated list of operators worth caring about for performance.
Three things are tracked per operator:

- **signature** in PostScript form: `inputs -> outputs`
- **binding** today: `fn` (functiontype, SLIFunction subclass),
  `op` (one of the operator marker typeids dispatched inline by
  the interpreter switch), or `trie` (TrieType wrapping typed
  leaves and dispatched via TypeNode lookup).
- **status**: where the body currently lives and whether it has
  been compacted in Stage 9.

A "hot" operator is one that the dispatcher sees a lot. Three
sources rank them:

1. The B-series microbenchmarks under `bench/sli/` —
   add, pop, dup, exch, for, repeat, ifelse.
2. `SLI3_STATS=1 ./build/sli3 < script.sli` on
   representative scripts. The bootstrap alone names about
   90 distinct operators; everyday user code adds dict /
   dictstack pressure (`get`, `put`, `known`, `def`,
   `begin`/`end`).
3. The GhostScript main-switch — which operators they thought
   were worth a direct case in `interp.c`'s big switch (see
   notes in `interp.c` around line 1213+: tx_op_add, tx_op_pop,
   tx_op_dup, tx_op_exch, tx_op_index, tx_op_eq, tx_op_if,
   tx_op_ifelse, tx_op_for, tx_op_repeat, tx_op_loop, ...).

The intersection of those three is the catalog below.

## How to read the status column

- ✅ compact — single C++ function, no trie. Numeric arms inline.
- ✅ inlined — dispatcher case in `execute_dispatch_`, no SLIFunction call.
- ⚠ trie — still goes through a trie. Candidate for compaction.
- ⚠ fn — SLIFunction with a single body, but the body is busy
  and/or has overhead worth measuring (e.g. multiple typed leaves
  bundled by a trie wrapper that no longer exists).
- 🔲 not started — no compaction effort yet.

## Tier 1 — operators in the dispatcher hot path

These fire on every iteration of a tight loop. Their per-call
cost shows up at percent level in B1/B2/B3.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `repeat` | `int proc repeat -> -` | fn | ✅ inlined (irepeattype) | Iter case body in `execute_dispatch_`. |
| `for` | `init step lim proc for -> ...` | fn | ✅ inlined (ifortype) | Iter case body. |
| `loop` | `proc loop -> -` | fn | 🔲 fn | LoopFunction is generic; no super-instruction. |
| `forall` | `coll proc forall -> -` | fn | ✅ compact | ForallFunction dispatches array/string to C++ leaves; for dict+proc it pushes the SLI-defined /forall_di to the e-stack. |
| `pop` | `any -> -` | fn | ✅ compact | PopFunction is a single function. |
| `dup` | `any -> any any` | fn | ✅ compact | DupFunction is a single function. |
| `exch` | `a b -> b a` | fn | ✅ compact | ExchFunction. |
| `index` | `... n index -> ... a_n` | fn | ✅ compact | Takes an int count. |
| `count` | `... -> ... n` | fn | ✅ compact | |
| `clear` | `... -> -` | fn | ✅ compact | |
| `rot` | `c b a rot -> a c b` | fn | ✅ compact | |
| `rollu` / `rolld` | `c b a -> b a c` / `a c b` | fn | ✅ compact | |
| `roll` | `... n k roll -> ...` | fn | ✅ compact | Two int args. |
| `if` | `bool proc if -> ?` | fn | 🔲 fn | IfFunction; cold per-call work is the bool check. |
| `ifelse` | `bool ptrue pfalse ifelse -> ?` | fn | 🔲 fn | IfelseFunction. |

## Tier 2 — arithmetic and comparison (compacted in Stage 9)

These were trie-bound (4 numeric arms + optional string arm)
before Stage 9. They are now single C++ functions that
type-switch inline. The trie typed leaves (e.g. `add_ii`,
`gt_ss`) remain registered so `addtotrie` users still work.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `add` | `n1 n2 -> n3` | fn | ✅ compact | 4 arms; int+int uses checked_add. |
| `sub` | `n1 n2 -> n3` | fn | ✅ compact | |
| `mul` | `n1 n2 -> n3` | fn | ✅ compact | |
| `div` | `n1 n2 -> n3` | fn | ✅ compact | DivisionByZeroError if n2 == 0. |
| `mod` | `i1 i2 -> i3` | fn | ✅ compact | Integer-only (1 arm). |
| `max` | `n1 n2 -> n3` | fn | ✅ compact | Wider operand wins type. |
| `min` | `n1 n2 -> n3` | fn | ✅ compact | |
| `gt`/`lt`/`geq`/`leq` | `a b -> bool` | fn | ✅ compact | 4 numeric arms + string arm. |
| `eq` | `any any -> bool` | fn | ✅ compact | Token::operator==. |
| `neq` | `any any -> bool` | fn | ✅ compact | |
| `and`/`or`/`xor` | `b b -> b` / `i i -> i` | fn | ✅ compact | Bool or bit-int. |
| `not` | `bool -> bool` / `int -> int` | fn | ✅ compact | |
| `sin`/`cos`/`asin`/`acos`/`exp`/`ln`/`log`/`sqr`/`sqrt` | `num -> num` | fn | ✅ compact | Int auto-promotes to double. |
| `pow` | `base exp -> result` | fn | ✅ compact | 4 arms with int-to-double promotion. |

## Tier 3 — container reads / writes

These are very common in user code. `get`/`put`/`length` are
still trie-dispatched today and should be the next compaction
batch.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `length` | `coll -> int` | fn | ✅ compact | LengthFunction dispatches by collection tag to length_a / length_s / length_d / length_p / length_lp. |
| `get` | `coll key -> elt` | fn | ✅ compact | GetFunction switches on (coll.tag, key.tag); arms: get_a (array+int), get_a_a (array+array), get_s (string+int), get_d (dict+lit), get_p/lp (proc/litproc+int). |
| `put` | `coll key elt -> coll` | fn | ✅ compact | PutFunction mirrors `get`; element-type arm is anytype so the typed leaf handles it. |
| `getinterval` | `coll idx n -> sub` | fn | ✅ compact | Array/string dispatcher in C++. |
| `join` | `coll1 coll2 -> coll3` | fn | ✅ compact | Array/string/procedure. |
| `first` / `last` | `coll -> elt` | trie | 🔲 trie | Rare; not urgent. |
| `append` | `coll elt -> coll'` | trie | 🔲 trie | |
| `prepend` | `elt coll -> coll'` | trie | 🔲 trie | |
| `reverse` | `coll -> coll'` | trie | 🔲 trie | |
| `empty` | `coll -> bool` | trie | 🔲 trie | empty_a / empty_s / empty_d. |
| `cva` | `coll -> array` | trie | 🔲 trie | cva_d for dict, others. |
| `size_a` / `size_s` | `coll -> int` | fn | ✅ compact | Typed leaves. |

## Tier 4 — dictionary / dictstack operations

The dict layer carries a lot of bootstrap traffic and shows up
in any introspection or definition pattern. `def`, `lookup`,
`known`, `begin`, `end` are the most common.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `def` | `lit any def -> -` | fn | ✅ compact | DefDispatchFunction picks 2-arg vs 3-arg form by reading the ostack tags; 2-arg -> DefFunction (C++ leaf); 3-arg (lit array obj) -> /:def_ SLI proc via baselookup + estack push. |
| `def_` | `lit any def_ -> -` | fn | ✅ compact | Raw bind (no array-defs-with-typecheck). Single function. |
| `:def_` | `lit array proc :def_ -> -` | fn | ✅ compact | Typechecked def. |
| `load` | `lit -> any` | fn | ✅ compact | Dictstack lookup, raises UndefinedName if missing. |
| `lookup` | `lit -> any true | false` | fn | ✅ compact | Lookup with present/absent flag. |
| `known` | `dict lit -> bool` | fn | ✅ compact | Single binding (no trie). |
| `where` | `lit -> dict true | false` | fn | ✅ compact | Searches dictstack. |
| `undef` | `dict lit -> -` | fn | ✅ compact | |
| `begin` | `dict begin -> -` | fn | ✅ compact | Push dict on dictstack. |
| `end` | `end -> -` | fn | ✅ compact | Pop dictstack. |
| `dict` | `int dict -> dict` | fn | ✅ compact | Construct empty dict with hint. |
| `currentdict` | `currentdict -> dict` | fn | ✅ compact | |
| `<<` / `>>` | `-> mark` / `mark k v ... >> -> dict` | fn | ✅ compact | DictconstructFunction. |
| `cleardict` | `dict cleardict -> -` | fn | ✅ compact | |
| `clonedict` | `dict -> dict'` | fn | ✅ compact | |
| `keys` | `dict -> array_of_lits` | fn | ✅ compact | |
| `values` | `dict -> array_of_tokens` | fn | ✅ compact | |
| `cva_d` | `dict -> array_of_kv_arrays` | fn | ✅ compact | |
| `info_d` / `topinfo_d` / `info_ds` | `dict -> -` | fn | ✅ compact | Pretty-prints. |

## Tier 5 — type / conversion / introspection

Used in nearly every script for type-checking, casting, and
metaprogramming.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `type` | `any -> /typename` | fn | ✅ compact | Returns literal of typename. |
| `typeinfo` | `any -> /typename` | fn | ✅ compact | NEST compat alias. |
| `cvi` / `cvd` | conversions | fn | ✅ compact | CviFunction / CvdFunction switch on source-type tag; int<->double<->string arms. Identity arm preserved (e.g. /cvi on integertype). |
| `cvs` | any -> string | n/a | n/a | Not currently bound at /cvs. Use cvs_x leaves directly. |
| `cvlit` / `cvx` / `cvn` | exec-bit / type toggle | fn | ✅ compact | CvlitFunction / CvxFunction / CvnFunction switch on the operand type. The stringtype arm of cvx/cvlit/cvn depends on cst/cvn_s which are stubbed Unimplemented upstream -- baselookup pushes them so the same error fires. |
| `cvn` | `string -> /literal` | fn | ✅ compact | |

## Tier 6 — control flow primitives

These already live as operator markers (dispatched inline by
the interpreter's main switch — no SLIFunction call). Listed
here because they appear at the top of stats output and form
the bottom of the iteration story.

| Op | Signature | Binding | Status | Notes |
|---|---|---|---|---|
| `::iiterate` / `::executeprocedure` | internal proc-body driver | op | ✅ inlined | iiteratetype dispatcher case. |
| `::irepeat` / `::repeat` | repeat continuation | op | ✅ inlined | irepeattype case. |
| `::ifor` / `::for` | for-loop continuation | op | ✅ inlined | ifortype case. |
| `::iforall_a` | forall continuation | op | ✅ inlined | iforalltype case. |
| `quit` | quit interpreter | op | ✅ inlined | quittype case. |
| `mark` | push mark on stack | op | ✅ inlined | marktype is a literal. |

## What to compact next

Looking only at things still marked ⚠/🔲:

1. **`forallindexed`** (Tier 1): companion to `forall`, same
   2-arm shape (array/proc, string/proc). Same recipe.
2. **`empty` / `cva` / `first` / `last` / `append` / `prepend`
   / `reverse`** (Tier 3): all still trie. Lower hit rates than
   the already-compacted ops so deferable.
3. **`if` / `ifelse`** (Tier 1): each is one SLIFunction. The
   virtual call to a single dominant target is essentially free
   on M2 (experiments with super-instructions did not pay off),
   so leave as-is unless a future architecture change reopens
   the question.

## Methodology note

The frequency numbers come from a 100M-iteration B3 run plus
the bootstrap that precedes it; bootstrap ops are inflated
toward the bottom of the table. For a steady-state measurement,
run a non-trivial SLI script under `SLI3_STATS=1` and read off
the top of the output:

```sh
SLI3_STATS=1 ./build/sli3 < my_script.sli 2>&1 \
  | sed -n '/operator usage/,/Allocations/p'
```

When picking compaction candidates, weight by both call
frequency and per-call cost — a 50-ns operator called 1000
times is cheaper to compact than a 10-ns operator called 100k
times. The dispatcher fast paths (Stage 9 + Stage 9-bench-run
iter-case inlining) already cover the cheapest, hottest ops.

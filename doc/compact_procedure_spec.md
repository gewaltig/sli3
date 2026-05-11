# Dispatcher restructure — closing the B2b gap to gs

Self-contained spec for the next dispatcher-perf change in sli3.
Supersedes prior "compact procedure storage" drafts (2026-05-08,
2026-05-11). Revision date: 2026-05-11.

Reference: gs 10.07 source under `/tmp/ghostpdl-10.07/psi/`
(`interp.c`, `interp.h`, `iref.h`, `zcontrol.c`) and
`Resource/Init/gs_init.ps`. Cross-checked against sli3's
`src/interpreter/sli_interpreter.cpp`, `src/builtins/
sli_builtins.cpp`, `lib/sli/sli-init.sli`.

## Origin of the gap

B2b (`{1 1 add pop} bind repeat × 100M`): sli3 1.91 s, gs 1.23 s.
At 4 op-dispatches per iter × 100 M iters that is ~4.8 ns/op
on sli3 and ~3.1 ns/op on gs — ~1.7 ns/op gap. The previous
drafts attributed this to "per-operator estack push" alone.
That is one cause among several, and reading gs's `interp()`
shows the gap is more structural than I'd assumed.

## How gs's dispatcher actually works

The hot path in gs is a single function, `interp()` in
`psi/interp.c:947–1965`. Inside it:

```c
register const ref_packed *iref_packed = (const ref_packed *)pref;
register int icount;
register os_ptr iosp = osp;
register es_ptr iesp = esp;

top:
  switch (r_type_xe(iref_packed)) {
  case plain_exec(tx_op_add):
    osp = iosp;
    if ((code = zop_add(i_ctx_p)) < 0) return_with_error_tx_op(code);
    iosp--;
    next_either();   /* advance iref_packed, goto top */
  case plain_exec(tx_op_pop):
    if (iosp < osbot) return_with_error_tx_op(gs_error_stackunderflow);
    iosp--;
    next_either();
  case plain_exec(t_integer):
    ...
    break;          /* falls into bot: which does next() */
  case plain_exec(t_operator):
    osp = iosp;
    switch (code = call_operator(real_opproc(IREF), i_ctx_p)) {
        case 0: case 1:
            iosp = osp;
            next();
        ...
    }
    ...
  }
bot: next();
  /* next() = if(--icount > 0) { iref_packed = IREF_NEXT(iref); goto top; } else goto out; */

out:
  if (!icount) { iesp--; iref_packed = IREF_NEXT(iref_packed); goto top; }
up:
  if (!r_is_proc(iesp)) { SET_IREF(iesp--); icount = 0; goto top; }
  SET_IREF(iesp->value.refs); icount = r_size(iesp) - 1;
  if (icount <= 0) { iesp--; if (icount < 0) goto up; }
  goto top;
```

Three things are doing the heavy lifting:

**(1) The procedure-body walk is the dispatcher's primary mode.**
`iref_packed` is a register-held pointer that walks through the
current procedure body. Each operator dispatch ends in `next()`
or `next_either()`, which advances `iref_packed` and `goto`s
right back to `top:`. There is no outer "iter case" wrapper that
re-decides what kind of body iteration this is on every token.
The dispatcher is iterating *by default*; the e-stack only
records the *transitions* into nested bodies, not every step.

**(2) Hot operators are inlined into the main switch.**
`tx_op_add`, `tx_op_pop`, `tx_op_dup`, `tx_op_exch`, `tx_op_if`,
`tx_op_ifelse`, `tx_op_index`, `tx_op_roll`, `tx_op_sub`,
`tx_op_def` each have their body (3–20 lines of C) inlined as a
switch arm in `interp()`. No virtual call, no function-pointer
indirection. Other operators dispatch through
`call_operator(real_opproc(IREF), i_ctx_p)` (one function-
pointer call), but still without an estack push for the
operator itself.

**(3) Refs are not pushed onto the e-stack per-operator.** The
"current operator" is the register-held `iref_packed`. When an
error occurs (label `rwe:` at `interp.c:1929`), gs captures
`ierror.obj = IREF` — that becomes the `/command` entry in
`$error`. The e-stack contains *procedure frames*
(each holding `(refs pointer, count remaining)`) and
*continuation operators* (`repeat_continue`, `for_pos_int_
continue`, `loop_continue`), never individual hot-op refs.

A few more nuts and bolts that matter:

- **Packed refs.** `iref_packed` may point at a 2-byte
  `ref_packed` slot (when the procedure body is a `t_mixedarray`
  / `t_shortarray`) or at a 16-byte full `ref` (`t_array`).
  Small ints, names, and operator references fit in 2 bytes
  inline; values that don't fit are stored as full refs in a
  side table and referenced via the packed slot. The
  `r_is_packed` test in the dispatcher branches between the two
  cases for `next()`. The 16-byte `ref` is the same size as
  sli3's `Token`.
- **Tail call on body end.** When `icount == 0` after `next()`,
  the dispatcher pops the e-stack frame (`iesp--`) and continues
  walking the *outer* frame's body directly via
  `iref_packed = IREF_NEXT(iref_packed); goto top;`. There is no
  "I am inside iiterate, let me decrement my counter, repeat the
  case" round-trip.
- **`repeat` / `for` / `loop` are C operator functions, not
  type-marker cases.** `zrepeat` (`zcontrol.c:465`) sets up an
  e-stack frame `[mark, count, proc, repeat_continue]` then
  jumps to `repeat_continue`, which copies the proc onto the
  e-stack and returns `o_push_estack`. The dispatcher's `up:`
  label enters the proc body and walks it via `iref_packed`.
  When the body ends, control returns to `repeat_continue`,
  which decrements the count and pushes again or returns
  `o_pop_estack`.

## How sli3's dispatcher works today

`SLIInterpreter::execute_dispatch_` (`sli_interpreter.cpp:726–
1182`) drives a single top-level `switch(tag())` on the e-stack
top. The dispatcher's "I'm walking a procedure body" mode is
implemented as a *case in that switch*: when the e-stack top is
`iiteratetype` / `irepeattype` / `ifortype` / `iforalltype`, the
case body reads `proc` and `pos` from estack picks, fetches one
token, dispatches it inline (with fast paths for `functiontype`
and `nametype`), then either `goto`s to the start of that case
to continue, or `break`s to re-enter the outer switch.

```cpp
case sli3::irepeattype: {
    TokenArray *proc = execution_stack_.pick(2).data_.array_val;
start_repeat:
    long &pos = execution_stack_.pick(1).data_.long_val;
    if (proc->index_is_valid(pos)) {
        const Token &t = proc->get(pos++);
        if (t.is_executable()) {
            if (t.tag() == sli3::functiontype) {
                execution_stack_.push(t);   // (*)
                t.data_.func_val->execute(this);  // op self-pops via EStack().pop()
                if (execution_stack_.top().tag() == sli3::irepeattype) {
                    proc = execution_stack_.pick(2).data_.array_val;
                    goto start_repeat;
                }
                break;
            }
            // ... nametype fast path ...
        }
        // ... literal push ...
        goto start_repeat;
    }
    // ... loop tail: dec counter or pop frame ...
}
```

The push at `(*)` exists for two reasons today:
1. So `raiseerror(Name)` (`sli_interpreter.cpp:372`) and
   `get_current_name` (`:363`) can identify the failing op by
   reading `EStack().top().data_.func_val->get_name()`.
2. So an operator that *invokes nested execution* can leave a
   chain of operator Tokens on the e-stack — when op A's body
   does `baselookup("op-B").data_.func_val->execute(this)`, op
   B's Token sits on top of op A's during B's body.

Per `SLIFunction::execute` ends with `i->EStack().pop()` — there
are ~316 such sites across `src/builtins/`. The push + self-pop
pair *is* the per-operator estack bookkeeping.

## Cost decomposition of the ~1.7 ns/op gap

Reading the sli3 inner loop and gs's `interp()` side-by-side:

| Source | gs (ns/op) | sli3 (ns/op) | Δ |
|---|---:|---:|---:|
| Token / ref load | 0.4 (2-B packed common) | 0.8 (16-B Token) | +0.4 |
| Type/exec decode | 0.3 (`r_type_xe` byte read) | 0.6 (`tag()` shift + `is_executable` chained load) | +0.3 |
| Outer-loop overhead between tokens | ~0.2 (`next()` → `goto top`) | ~0.8 (`goto start_repeat` → re-pick of `proc`, `pos` from estack) | +0.6 |
| Per-op estack push | 0 | 1.8 (16-B Token copy onto e-stack vector) | +1.8 |
| Op body dispatch | 0.3 (inline switch arm) or 0.6 (`call_operator`) | 0.4 (vcall via SLIType→func_val) | varies |
| Op self-pop | 0 | 0.8 (vector pop_back of Token) | +0.8 |
| Op body work | ~1.4 (e.g. `iosp--; iosp[0] = iosp[0] + iosp[1];`) | ~1.2 (e.g. AddFunction::execute math) | -0.2 |
| **Total per op** | **~2.6–2.9** | **~6.4** | **~3.5–3.8** |

Per-iter on B2b (4 tokens): gs ~11–12 ns, sli3 ~19 ns. Matches
the observed B2b times (1.23 s / 1.91 s).

The "+1.8 + 0.8 = +2.6 ns from estack push/pop" line is real
but is roughly half of the gap. The *other* half is structural:
sli3's outer-loop overhead per token (re-picking `proc` and
`pos` from e-stack on every `goto start_repeat`) and the
chained-load type decode add up to ~0.9 ns/op. The packed
storage savings is ~0.4 ns/op for a body of small integers and
operator names; less for bodies with doubles or larger items.

## What it would take to close the gap

The three changes below correspond to three independent axes.
Each can ship on its own, but they compound: doing only one
captures only a fraction of the gap.

### Axis I — Inline the procedure-body walk into the dispatcher

The dispatcher's main loop becomes "walk `iref` through the
current body, dispatch each ref, advance `iref`". The e-stack
holds *procedure frames* (current body pointer + remaining
count) and *continuation operators*, not individual operator
Tokens.

This is structurally the same as gs's `interp()` topology. It
delivers:
- **Removes the outer-loop overhead between tokens** (no
  re-picking of `proc` / `pos` from e-stack per token; `iref`
  and `count` are register-held locals).
- **Removes the per-op estack push, automatically** — because
  the body-walk position *is* the current-op pointer. `raiseerror`
  reads it from the dispatcher's register state (or, more
  practically, the dispatcher writes it back to the iter
  frame's `pos` field just before raising).
- **Removes operator self-pop**, automatically — operators
  never push themselves in the first place.

This is the biggest single change. Expected B2b drop:
**~4 ns/iter** (1.91 s → ~1.50 s).

**Surface:**
- Rewrite of `execute_dispatch_` (`sli_interpreter.cpp:726–
  1182`). The outer switch becomes the proc-body walk loop with
  register-held `iref` and `count`. The four iter cases
  (`iiteratetype`, `irepeattype`, `ifortype`, `iforalltype`)
  collapse into "advance into body" prologue + "decrement
  counter / pop frame" epilogue. The `IiterateFunction`,
  `IrepeatFunction`, `IforFunction`, `IforallarrayFunction`
  classes (`sli_builtins.cpp:94, 175, 229, 287`) either go
  away entirely or shrink to thin wrappers used only via the
  `case sli3::functiontype:` path for the cold REPL entry point.
- `raiseerror(Name)` and `get_current_name` change source of
  truth from `EStack().top()` to a new "current ref"
  pointer/index. Two options:
  - **(I.a)** A single register-spilled field on
    `SLIInterpreter`: `Token const* current_ref_`. Dispatcher
    sets it before each op call; raiseerror reads it.
  - **(I.b)** No new field; raiseerror walks the e-stack
    looking for an iter frame and recovers
    `proc->get(pos-1)`. Cheaper to maintain but pays a small
    cost on the error path.
  Pick (I.b) — same semantics as gs's `IREF = ierror.obj`,
  and the proc / pos are already in the iter frame.
- **316 sites** in `src/builtins/` that end with
  `i->EStack().pop()`: most of them go away. The exceptions are
  operators that net-push onto the e-stack (e.g. `if`/`ifelse`
  push a procedure onto e-stack and return; their pop balances
  the operator's slot, but with the new ABI they push without
  needing to balance a self-pop because the dispatcher didn't
  push them either). Inventory before editing.
- `recordstacks` snapshot in `raiseerror(Name, Name)`
  (`sli_interpreter.cpp:~460`). Today it captures the e-stack
  contents verbatim. With the new ABI, the e-stack no longer
  contains operator Tokens, so the snapshot has a different
  shape. **This is actually the same shape as gs's `execstack`
  array — procedure frames + continuation operators, with the
  failing operator separately as `$errordict /commandname`.**
  No SLI-visible regression vs gs; sli3-internal users of the
  snapshot need to be audited (`test_errors.cpp`,
  `test_errors_dispatch.cpp`, `bench/sli/B6*.sli`).
- `backtrace` (`SLIFunction::backtrace` overrides for the four
  iter types) — already walk the iter frame, no change needed
  except the iter-type marker on the frame moves.

**Risks:**
- Nested-op error attribution: if op A's C body calls op B via
  `fn->execute(this)`, and B raises, the iter frame's
  `proc[pos-1]` points at the *outer* op (the slot in the
  procedure body that started the chain), not at op B. The
  failing op identity is degraded. Mitigations: (1) the
  inner-call sites in `src/builtins/` are explicit and few
  (~20-30 sites; `ifelse`'s "execute branch", `def`'s "trie
  forwarding", etc.); a one-pointer `inner_op_hint_` field
  populated by inner-callers fixes the attribution. (2) gs has
  the same property and ships with it.
- Backwards compat for `$errordict /estack` shape: if any user
  SLI code reads operator Tokens out of the snapshot, it
  breaks. Vendored `sli-init.sli` doesn't appear to (grep for
  `errordict /estack`); tests don't either. Confirm before
  shipping.

### Axis II — Inline hot operator bodies into the dispatcher

After Axis I, the dispatcher's main switch is the right place
to inline the bodies of hot operators. `pop`, `dup`, `exch`,
`add` (integer arm), `sub` (integer arm), `index`, `roll`,
`def`, `if`, `ifelse`, `for`, `repeat` all become switch arms
of 3-15 lines apiece. The rest of `SLIFunction` subclasses are
called through the function-pointer path.

This is gs's `tx_op_*` design. The implementation looks like:

```cpp
enum HotOpId : uint8_t {
    HOP_NONE = 0,
    HOP_ADD, HOP_SUB, HOP_DUP, HOP_POP, HOP_EXCH,
    HOP_INDEX, HOP_ROLL, HOP_IF, HOP_IFELSE,
    HOP_DEF, HOP_FOR, HOP_REPEAT,
    HOP_count
};

class SLIFunction {
    HotOpId hot_op_id_ = HOP_NONE;
public:
    HotOpId hot_op() const { return hot_op_id_; }
    ...
};
```

`SLIFunction::hot_op()` returns `HOP_NONE` by default;
subclasses for the inlined ops set it in their constructor.
The dispatcher reads it for `functiontype` refs and switches
on it before falling back to `fn->execute(this)`.

Expected B2b drop on top of Axis I: **~1-2 ns/iter**. Smaller
than Axis I because the vcall on M2 is already well-predicted;
the gain is from inlining the body next to the body-walk so the
compiler can fold the operand-stack pointer arithmetic across
op boundaries.

**Surface:**
- New `enum HotOpId` in `src/interpreter/sli_function.h`.
- `SLIFunction` gains a `hot_op_id_` member with a default of
  `HOP_NONE`. Subclasses for hot ops set it in their ctor.
- New `src/builtins/sli_op_bodies.h` with inline-able body
  functions or macros for the hot ops. The non-inlined
  `AddFunction::execute` etc. delegate to the same inline body
  so behaviour stays in sync.
- The dispatcher's "got a `functiontype` ref" path becomes:

```cpp
case fn_tag: {
    SLIFunction* fn = iref->data_.func_val;
    switch (fn->hot_op()) {
        case HOP_ADD: { /* inline body */ break; }
        case HOP_POP: { /* inline body */ break; }
        ...
        case HOP_NONE: default:
            fn->execute(this);
            break;
    }
    advance_iref(); goto top;
}
```

**Risks:**
- The body has to be kept in sync between the inline arm and
  the standalone `execute()`. Either factor into a `static
  inline` helper that both call, or accept the duplication
  (and add a parity test).
- Hot-op-id leakage: every place that constructs an
  `SLIFunction*` (e.g. `BindFunction`, `:def_`) has to thread
  the hot-op-id through. The default is `HOP_NONE`, so it's
  safe to forget — just won't inline.
- More cases in the switch can lengthen the jump table; profile
  to confirm M2 still produces a tight dispatch.

### Axis III — Compact procedure storage

After `bind`, the procedure body is stored as a packed array
of slots rather than a `std::vector<Token>`. This is the
original spec's compact-storage idea, now positioned as a
**third axis after** the dispatcher restructure rather than
the headline.

Once Axis I is in, the dispatcher's body walk is a thin
abstraction over "advance through this body's slots". It can
be polymorphic: the walking loop takes a body kind (TokenArray
or CompactProc) and dispatches accordingly. Slot decode is
slightly cheaper for compact bodies (single byte tag, no
chained type-load); cache footprint shrinks by 4× for typical
bodies.

#### Slot format

```cpp
enum CompactTag : uint8_t {
    CT_INT_SMALL  = 0,   // 24-bit signed int (range ~−8M..+8M)
    CT_BOOL,             // payload bit
    CT_FUNCTION,         // payload = index into fns[]
    CT_NAME,             // payload = Name handle (late-bind path)
    CT_LITERAL,          // payload = Name handle (executable=false)
    CT_DBL_INDEX,        // payload = index into doubles[]
    CT_NESTED_INDEX,     // payload = index into spilled[]
};

struct CompactSlot {     // 4 bytes
    CompactTag tag;
    uint8_t    aux;      // small-int high byte; reserved otherwise
    uint16_t   payload;
};

class CompactProc {
public:
    std::vector<CompactSlot>   slots;
    std::vector<double>        doubles;
    std::vector<SLIFunction*>  fns;
    std::vector<Token>         spilled;
    uint32_t                   refs = 1;
};
```

4 B/slot is the right size for sli3 (vs gs's 2 B): we don't
have 16-bit Name handles, our operator pointers don't fit in
14 bits, and 24-bit small ints cover the common case without
spilling.

Expected B2b drop on top of Axes I + II: **~0.5-1 ns/iter**.
The B2b body is 4 slots; cache footprint is already small. The
real gain is on bigger bound bodies (10-50 slots), where the
4× compression buys cache lines.

#### Compactor

C++ `BindFunction` replaces the SLI-defined `bind` (`lib/sli/
sli-init.sli:261-286`). Per-token compaction rules:

- `integertype` fitting 24-bit signed → `CT_INT_SMALL`.
- `integertype` too large → spill, `CT_NESTED_INDEX`.
- `doubletype` → `doubles.push_back`, `CT_DBL_INDEX`.
- `booltype` → `CT_BOOL`.
- `literaltype` → `CT_LITERAL`.
- `nametype` whose dictstack lookup resolves to `functiontype`
  → `fns.push_back`, `CT_FUNCTION`.
- `nametype` resolving to anything else (incl. `trietype`),
  unresolved, or non-callable value → `CT_NAME` (late-bind).
  Doc-comment: bind no longer replaces names whose values are
  tries — the trie still works via late lookup; this differs
  from NEST 2.x's `bind` which replaced trie names. **Decision
  point: do we accept this divergence, or extend the slot
  format with a `CT_TRIE` arm? Trie users were rare on the
  B-bench surface; keep simple for now.**
- `proceduretype` (nested proc) → recurse, push compacted
  result into `spilled` as a `compactproceduretype` Token,
  `CT_NESTED_INDEX`.
- everything else → spill the Token, `CT_NESTED_INDEX`.

Bind is idempotent on `compactproceduretype` input.

A new `compactproceduretype` enum slot is appended to
`sli_typeid` (`sli_type.h:21–51`). This bumps
`kSerializeVersion` (`sli_serialize.h:20`) to 2. The serialize
override re-emits the unpacked equivalent so the wire format
doesn't depend on the slot encoding.

#### Mutation

`put` / `append` etc. on a `compactproceduretype` Token first
decompact back to a `TokenArray` (retag the Token as
`proceduretype`), then perform the mutation. Re-binding
re-compacts.

## Implementation order

The three axes are independent; either Axis I or II can land
first. Axis III is the lowest priority because the win is
smallest.

**Recommended order:**

1. **Axis I first.** Biggest single win and the structural
   change that the other axes ride on. Without it, Axis II is
   harder (inlining bodies into a `case sli3::functiontype:`
   arm that runs once per inner-switch round-trip is messier
   than inlining them into a body-walk top-of-loop). Axis III
   without Axis I just changes storage; the per-token overhead
   stays.
2. **Axis II second.** Once the dispatcher is the body-walk
   loop, hot-op inlining is a focused additive change.
3. **Axis III last.** The cache-density win is small on the
   benchmark surface; the storage-format complexity is real;
   serialize/mutation/introspection have to be done right.
   Only worth doing if Axes I + II haven't fully closed the gap
   and a profile shows body load as the next hot spot.

**Sliced plan, one commit per slice:**

### Phase 1 — Axis I (dispatcher restructure)

1. Lift `iref` / `count` / `iosp` / `iesp` to register-held
   locals at the top of `execute_dispatch_`. Currently `iosp` /
   `iesp` are implicit via member accesses; explicit copies
   let the compiler keep them in registers across the loop.
2. Replace the four iter cases with a unified body-walk loop
   that runs at the top of the dispatcher. The iter cases'
   *entry* (push frame, set up counter) stays in the C++
   operators (`zrepeat` / `zfor` / `IforallarrayFunction`);
   the *body iteration* is owned by the dispatcher.
3. Move the operator-self-pop convention: change the
   dispatcher's `functiontype` arm to NOT push `t` onto
   e-stack. Change every `SLIFunction::execute` in
   `src/builtins/` to NOT call `i->EStack().pop()` at exit.
   This is ~316 mechanical edits; do it in batches per file
   with `test_errors_dispatch` running between batches.
4. Update `raiseerror(Name)` and `get_current_name` to recover
   the failing op from the iter frame's `proc[pos-1]`. Add
   `inner_op_hint_` for explicit inner-call sites.
5. Update `recordstacks` snapshot shape; update tests.
6. Run `bench/run.sh`. Expected B2b: ~1.50-1.55 s.

### Phase 2 — Axis II (hot-op inlining)

1. Add `HotOpId` enum and `SLIFunction::hot_op_id_` field.
2. Tag the constructors of the ~10 hottest operator classes
   with their `HotOpId`.
3. Add inline body helpers in `src/builtins/sli_op_bodies.h`;
   rewrite the non-inlined operator bodies to call them
   (single source of truth).
4. In the dispatcher's `functiontype` arm, switch on
   `fn->hot_op()` before falling through to `fn->execute`.
5. Run `bench/run.sh`. Expected B2b: ~1.30-1.40 s.

### Phase 3 — Axis III (compact storage)

1. `CompactProc` type + builder, unit-tested in isolation.
2. `compactproceduretype` enum slot + `CompactProcedureType`
   SLIType subclass. Bumps `kSerializeVersion`.
3. C++ `BindFunction` registered as `/bind` (replaces SLI
   `bind` in `sli-init.sli`).
4. Dispatcher gains a compact-slot decode in the body-walk
   loop.
5. Introspection (`length_p`, `get_p`, `cva`, `==` printing)
   handled by decompact-on-call, or by direct compact
   accessors where worth the code.
6. Mutation: decompact-on-write.
7. Serialization: emit as unpacked.
8. Update `doc/hot_ops.md`.

## Bench expectations

Predicted, to be confirmed:

| Bench | today | + Axis I | + I + II | + I + II + III |
|-------|------:|---------:|---------:|---------------:|
| B1    | 1.33 s | 1.30 s | 1.28 s | 1.27 s |
| B2    | 2.40 s | 2.05 s | 1.85 s | 1.80 s |
| **B2b** | **1.91 s** | **~1.55 s** | **~1.35 s** | **~1.30 s** |
| B3    | 2.10 s | ~1.70 s | ~1.55 s | ~1.50 s |
| B5    | 16.3 s | ~15.7 s | ~15.5 s | ~15.5 s |

If B2b after Axis I lands worse than 1.65 s, the cost model is
wrong. Profile with Instruments before continuing. Axis II is
cheap if I doesn't pan out; Axis III is large complexity vs
small return and should not start until I + II are known wins.

## What we keep — PostScript semantics

Late binding remains the default outside `bind`. A `nametype`
ref in a procedure body is re-resolved every time the body
runs, unless `bind` resolved it to a `functiontype` at
compaction time. The `CT_NAME` slot dispatches through
`lookup()` at iteration time; only the `CT_FUNCTION` slot
freezes the resolution.

Mutation correctness: `proc 0 5 put` followed by re-running
`proc` must reflect the put. Decompact-on-write covers it.

## Risks summary

- **Axis I missed self-pop**: an operator that no longer
  self-pops but is invoked via a path that still pushes will
  leak an e-stack frame per call. Caught by
  `test_dispatch_parity` (it dumps stacks). Add an assertion
  in the dispatcher's `default:` arm that the e-stack depth
  is balanced.
- **Axis I nested-op attribution**: op A's body calls op B via
  `fn->execute(this)`; B raises; `$errordict /commandname`
  reports A unless `inner_op_hint_` is wired. ~20-30 sites to
  audit in `src/builtins/sli_control.cpp`,
  `sli_typecheck.cpp`, `sli_container_ops.cpp`. gs has the
  same property and ships with it.
- **Axis I e-stack shape change**: `$errordict /estack`
  contains different ref kinds than today (no operator Tokens;
  only procedure frames and continuations). Same shape as gs.
  Audit vendored `*.sli` and tests; confirm no breakage.
- **Axis II body duplication**: the inline body and standalone
  `execute()` must stay in sync. Single-source-of-truth via
  `static inline` helpers; add a parity test that exercises
  both paths with the same inputs.
- **Axis III mutation correctness**: any code path that
  writes `data_.array_val` on a `compactproceduretype` Token
  is a bug. Audit all proc-Token write sites.
- **Axis III memory pressure**: compact bodies add allocations
  (slots vector + side pools). For one-shot procedures that's
  overhead. Keep `bind` opt-in.

## File pointers (current sli3 layout)

| What | Path |
|------|------|
| Token / SLIType layout | `src/types/sli_token.h`, `src/types/sli_type.h` |
| TokenArray | `src/containers/sli_array.h` |
| SLIFunction base | `src/interpreter/sli_function.h` |
| Dispatcher | `src/interpreter/sli_interpreter.cpp:726` (`execute_dispatch_`) |
| Four iter cases | `:858, 928, 1001, 1068` |
| raiseerror / get_current_name | `:363, 372, 410` |
| `IiterateFunction` etc. continuation operators | `src/builtins/sli_builtins.cpp:94, 175, 229, 287` |
| Existing SLI `/bind` | `lib/sli/sli-init.sli:261–286` |
| Serialization version | `src/serialize/sli_serialize.h:20` |
| Bench harness | `bench/run.sh`, `bench/sli/B*.sli`, `bench/ps/B*.ps` |
| Op catalog | `doc/hot_ops.md` |

## File pointers (gs reference)

| What | Path |
|------|------|
| Ref layout | `psi/iref.h:412–457` |
| Main interpreter loop | `psi/interp.c:947–1965` |
| Hot-op inlining | `psi/interp.c:1213–1317` (`tx_op_add`, ...) |
| Generic operator dispatch | `psi/interp.c:1362` (`t_operator` case) |
| Body-walk macros | `psi/interp.c:1066–1083` (`next`, `next_either`) |
| Loop ops (repeat / for / loop) | `psi/zcontrol.c:288, 465, 503` |
| Error capture (IREF → command) | `psi/interp.c:1925–1965` (`rwe:` / `rwei:`) |
| `.errorhandler` building `$error` | `Resource/Init/gs_init.ps:1158–1201` |
| Stack snapshot during error | `Resource/Init/gs_init.ps:1171–1184` |

## Out of scope

- 2-byte slot encoding (gs's `ref_packed`). 4 B works for sli3
  (our handles don't fit gs's 14-bit operator id space).
- Computed-goto threaded dispatch. After Axis I + II the main
  switch is already a tight jump table on M2; threaded code is
  unlikely to win until measurements say otherwise.
- Compaction of literal arrays. PostScript `bind` doesn't touch
  them; we don't either.
- Re-introducing per-operator stack tracking for nested-call
  attribution beyond the `inner_op_hint_` shim. Matches gs's
  behaviour; users debugging deep nested-C-call attribution
  use the source-level debugger.

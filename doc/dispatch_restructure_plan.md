# Axis I — dispatcher restructure: implementation plan

Plan for executing Axis I of `doc/compact_procedure_spec.md`:
restructure `execute_dispatch_` so the procedure-body walk is
the dispatcher's primary mode (gs's `interp()` topology), and
drop the per-operator e-stack push/pop. Target: B2b 1.91 s →
~1.55 s, plus modest gains on B1/B2/B3.

This is the largest sli3 change since the Stage 5 bootstrap.
The plan slices it into eight independently buildable,
testable, and revertable steps. Each step has an explicit
test/bench gate.

Reference: `doc/gs_reference.md` (gs's actual implementation),
`doc/compact_procedure_spec.md` (the why and the cost model).

---

## Why slice this?

The surface includes:
- The dispatcher (`sli_interpreter.cpp:726-1182`, ~450 lines).
- ~316 `EStack().pop()` sites in `src/builtins/`.
- `raiseerror` / `get_current_name` / `recordstacks` snapshot
  (`sli_interpreter.cpp:363-460`).
- The four iter operators (`IiterateFunction`, `IrepeatFunction`,
  `IforFunction`, `IforallarrayFunction` —
  `sli_builtins.cpp:94-330`).
- `stop` / `stopped` walking the e-stack
  (`src/builtins/sli_control.cpp` — `stopfunction`,
  `stoppedfunction`).

Doing it all in one PR would be unreviewable and unrevertable.
Slicing gives:

- **Bench data per step** — confirms the cost model before
  committing to the next step.
- **Test-pass gate per step** — catches regressions where the
  blast radius is small.
- **Revert path** — if a slice goes wrong, only the last slice
  reverts, not the whole project.

The slices are ordered so each compiles, runs all existing
tests, and either improves or is neutral on the bench. Some
slices are pure refactors (no bench change); they exist to
break the larger change into reviewable chunks.

---

## State invariants — what must hold across the restructure

The interpreter's snapshot/restore contract today is simple:
`(operand_stack, execution_stack, dictionary_stack)` is the
complete program state at any *logical boundary* (between
dispatcher iterations, in an error handler, anywhere a
PostScript operator can observe state). The `src/serialize/`
protocol relies on this. So does `recordstacks`, `stop` /
`stopped` unwinding, `execstack` / `dictstack` snapshots, and
any future "freeze and resume" use case.

Axis I introduces register-held working state (`iref`,
`icount`, `iosp`, `iesp`, `current_op_`). For the three-stack
snapshot to remain authoritative, the slices below must
preserve four invariants. Treat these as correctness
requirements, not perf optimisations — violating them breaks
snapshot/restore and error reporting, not just the bench.

### Invariant 1: stacks are authoritative at logical boundaries

At any point where user PostScript code can observe state
(between operator dispatches, in error handlers, during
`execstack` / `recordstacks` snapshot, before any `throw` the
dispatcher catches), the three stacks must hold the full
record of execution state. The register-held working copies
of stack pointers are *cached values* of the underlying
member fields; the stacks themselves are the source of truth.

This is already the contract today; Axis I extends it by
adding the register copies introduced in Slice 3 and the
body-walk locals introduced in Slice 8.

### Invariant 2: sync register state before user-observable boundaries

The dispatcher must write register-held working state back to
the underlying data structures **before**:

- every `fn->execute(this)` call (the op may call `execstack`,
  trigger `raiseerror`, or be the snapshot operator itself);
- every `throw` site (the catch handler reads the e-stack);
- every exit from the dispatcher loop (top-level returns).

Concretely:

| Register-held | Source of truth | Sync before op call |
|---|---|---|
| `iosp` (Slice 3) | `operand_stack_.p` | `operand_stack_.p = iosp;` |
| `iesp` (Slice 3) | `execution_stack_.p` | `execution_stack_.p = iesp;` |
| `iref`, `icount` (Slice 8) | active iter frame's `pos` field on e-stack | `store_state(iref, icount)` writes `pos` |
| `cycle_count` local (existing) | `cycle_count_` member | `cycle_count_ = local_cycles;` |

After the operator returns, the dispatcher re-reads
(`load_state` / `iosp = &operand_stack_.top()+1` /
`iesp = &execution_stack_.top()+1`) because the op may have
modified the underlying state (`exec`, nested iter setup,
push, pop, …).

This invariant generalises the per-slice "sync before
throws" hazard mentioned in Slice 3 and Slice 8 into a
single contract that applies to every op call after the
slices land.

### Invariant 3: `current_op_` is transient, not state

`current_op_` (Slice 1) is non-null only inside an
`fn->execute(this)` call. It is set by the dispatcher
immediately before the call and cleared immediately after.
Between dispatcher iterations and in error handlers (after
`raiseerror` returns), it is null.

Consequences:

- **Snapshot/restore does not include `current_op_`.** A
  snapshot taken via a PostScript operator (which runs
  inside `fn->execute`) sees `current_op_ = self` — useless
  information that doesn't need preservation. The fresh
  interpreter on restore doesn't need `current_op_` set; the
  resuming dispatcher re-establishes it on its next op call.
- **Error reporting uses `current_op_` (via
  `get_current_name`) only at the moment `raiseerror` fires,
  which is inside the operator's execute. By the time the
  error handler proc runs, `current_op_` has been recorded
  into `$errordict /commandname` and cleared.**

### Invariant 4: e-stack shape changes — `/commandname`, not `/estack`

After Slice 7, the e-stack does **not** contain individual
operator Tokens between procedure-body refs. It contains
procedure-body frames (proc + pos + iter marker), continuation
operators, and `stopped` / `for` / oparray marks. This is
gs's shape.

Any code (SLI- or C++-level) that today reads operator names
out of an e-stack snapshot must instead read
`$errordict /commandname`. Audit before slice 7:

```sh
grep -rn 'errordict.*estack\|estack.*errordict\|$error.*estack' lib/sli/ tests/
```

Expected hits: 0. If any exist, decide per site whether to
update the consumer or to synthesise a per-op record when
building the snapshot.

`recordstacks` in `raiseerror(Name,Name)` snapshot logic
itself is *internal* to sli3, not user code; it can be
updated to produce the new shape without semantic regression.
The wire-visible record is `$errordict /commandname` (the
failing op name) plus `$errordict /command` (the failing
object) — both already populated today.

### Why this matters

Without invariants 1 + 2, a snapshot taken from inside an
operator (e.g. via a `freezestate` operator that serializes
the three stacks) would record stale `pos` for the active
iter frame. On restore, the dispatcher would resume body-
walking from the wrong position — either re-executing
already-dispatched refs, or skipping refs that should run.

Without invariant 3 explicitly documented, future code
might assume `current_op_` is preserved across snapshot
boundaries — leading to bugs where a restored interpreter
attributes the wrong op to errors.

Without invariant 4 acknowledged up-front, slice 7's e-stack
shape change is a silent breakage for any user SLI code
that introspects `$errordict /estack`.

### Test plan for the invariants

In addition to the per-slice test gates already specified:

1. **Round-trip parity test.** A new test (`tests/
   test_dispatch_snapshot.cpp`) that:
   - Runs a PostScript script up to a known mid-execution
     point (e.g. inside a `for` loop at iteration N).
   - Captures `(o-stack, e-stack, dict-stack)` via serialize.
   - Restores into a fresh interpreter.
   - Runs both interpreters to completion.
   - Asserts identical o-stack contents at the end.
2. **Body-position snapshot.** Inside a procedure body
   `{ a b c snapshot_here d e }`, when `snapshot_here` runs:
   the iter frame's `pos` field on the captured e-stack must
   point at `d` (the next ref to dispatch), not at
   `snapshot_here`. Verifies invariant 2 for `store_state`.
3. **E-stack shape audit.** After every slice that touches
   the e-stack shape (4, 7, 8): run an SLI script that
   inspects `$errordict /estack` after an error inside a
   nested procedure. Compare shape; document the differences.
   No per-op Tokens after slice 7; only iter frames,
   continuations, and marks.

---

## Re-costing (2026-05-12)

After Slice 0 baseline + Slice 1 attempt (reverted) + Slice 3 land
+ B7-B10 organic benchmarks (commit `31d6518`), the original cost
model needs revision. Two findings:

**(1) Slice 1/4's "save on the push" prediction was wrong.** The
plan expected the sentinel-replaces-push pattern to skip a
refcount-virtual-call. But `Token::Token(Token const&)` already
has a `needs_refcount()` short-circuit (`sli_token.h:223`), and
`FunctionType` doesn't override refcount methods. So pushing a fn
Token already skips the virtual; the sentinel can't beat that.
Slice 1 reverted at +4.8 % B2b regression (commit `5fe3d6b`).

**(2) Organic workloads have very different bottlenecks than
B1-B5.** Bench standing at HEAD (`31d6518`):

| Bench | sli3 | gs | sli3 vs gs |
|-------|-----:|---:|-----------:|
| B1 1 pop                  | 1.22 | 1.41 | **−13 %** |
| B2 1 1 add pop            | 2.33 | 2.88 | −19 % |
| B2b bound proc            | 1.90 | 1.28 | **+48 %** |
| B3 nested for             | 2.03 | 2.70 | −25 % |
| B5 dict alloc + lookup    | 1.70 | 2.65 | −36 % |
| B7 bubble sort            | 3.29 | 2.69 | **+22 %** |
| B8 insertion sort         | 2.39 | 1.79 | **+34 %** |
| B9 recursive fib(28)      | 3.83 | 2.58 | **+48 %** |
| B10 matmul 50x50          | 3.13 | 2.75 | **+14 %** |

Profile data (sample(1), 5s, 1ms intervals, on a 5x-longer run of
each workload):

### B2b — bottleneck: pure dispatcher overhead

```
execute_dispatch_   82 %
AddFunction          9 %
PopFunction          9 %
(nothing else)
```

What helps: dispatcher inner-loop reduction (Slice 8 inline body
walk). The other slices are noise here.

### B9 fibonacci — bottleneck: recursion + procedure dispatch

```
execute_dispatch_                 54 %
vector<Token>::erase              13 %   <-- iter-frame popping
ArrayType::remove_reference        6 %   <-- refcount on proc pop
DictionaryStack::lookup            6 %   <-- name dispatch (un-bound)
IfelseFunction                     5 %
Name::handleTableInstance_         4 %
LtFunction / DupFunction /         7 %
  SubFunction / AddFunction / ExchFunction
```

What helps:
- Axis I (drop per-op push + drop self-pop): halves the 13%
  `vector::erase` cost → ~−6 % B9.
- pvalue cache (P1): eliminates the 6% `DictionaryStack::lookup`
  → ~−5 % B9 plus the `handleTableInstance_` cost.
- Axis II hot-op inlining: replaces 7% of virtual-call op bodies
  with inline switch arms → ~−3 % B9.

Total Axis I + II + P1 potential on B9: ~−15 %.

### B10 matmul — bottleneck: dictionary pressure

```
execute_dispatch_              36 %
vector<Token>::erase           11 %
DictionaryStack::lookup        10 %
Name::handleTableInstance_      7 %
DictionaryStack::def            6 %
GetArrayLikeFunction (get_a)    6 %
Dictionary::insert              6 %
ArrayType::remove_reference     4 %
Name::num_handles               3 %
(rest small)
```

The dict path (lookup + def + insert + Name handling) is **~32 %**
of total time. The `/sum sum ... add def` loop body hits dict
insert/lookup every inner iteration.

What helps:
- **pvalue cache (P1)** dominates: ~15-20 % on B10 directly.
- Axis I: halves vector::erase (~−5 %).
- Axis II: get_a body is 6% but hard to inline (not in the hot-op
  whitelist today).

Total potential on B10: ~−20-25 %.

## Re-prioritised slice ordering

Given the data, the previous Slice 4→5→6→7→8→9 ordering is
suboptimal. The new ordering:

| Order | Old | New | Why |
|------:|-----|-----|-----|
|   1   | —   | **Slice 11 (P1 pvalue cache)** | Biggest single win on B5/B7/B9/B10. Independent of Axis I. Lower risk than dispatcher restructure. |
|   2   | 4-7 | **Slice 4-7 combined as atomic Axis I** | The original 4→7 progression's bench gates can't be met by any single slice in isolation (each pays setup cost; only the combined net is favorable). Bundle them as one logical change with per-file commits internally; bench gate only at the end. |
|   3   | 8   | **Slice 8 inline body walk** | Same as before. Highest risk, highest reward on B2b. |
|   4   | 9   | Slice 9 (continuation ops) | Optional cosmetic cleanup. |

### Per-slice revised cost model

| Slice | Old expected | New expected | Decision |
|-------|--------------|--------------|----------|
| 1 | 0 ± 2 % | +5 % B2b alone, recovered with 4-7 | Deferred into 4 |
| 2 | 0 | 0 (audit-only) | ✓ Done |
| 3 | −0.05-0.10 s B2b | −0.14 s B2b, −0.22 s B1, −0.20 s B3 measured | ✓ Done |
| 4 alone | −0.05 s B2b | +5 % B2b (sentinel doesn't beat needs_refcount short-circuit) | Must combine with 5+6+7 |
| 5+6+7 alone | −0.15-0.25 s B2b | Saves the `vector::erase` work the slice-4 push added; net ~0 to −5 % on B2b | Combine with 4 |
| 4+5+6+7 combined | n/a | net −5 to −10 % on B2b; bigger on B7/B9 (Axis I cleanup) | New plan |
| 8 | −0.10-0.15 s B2b | −15 to −20 % on B2b (inline body walk + multi-level TCO) | Keep |
| 9 | 0 | 0 (cosmetic) | Optional |
| **11 (new)** | n/a | −15-20 % on B5/B7/B10; −5 % on B9 | New top priority |

Expected end-state after Slice 11 + combined-4-7 + Slice 8:

| Bench | Now | Expected | Δ |
|-------|----:|---------:|--:|
| B1    | 1.22 | 1.10 | −10 % |
| B2    | 2.33 | 2.00 | −14 % |
| B2b   | 1.90 | 1.40 | **−26 %** (gs is 1.28) |
| B3    | 2.03 | 1.75 | −14 % |
| B5    | 1.70 | 1.35 | −21 % |
| B7    | 3.29 | 2.65 | **−19 %** (gs is 2.69; tied) |
| B8    | 2.39 | 2.05 | −14 % |
| B9    | 3.83 | 3.10 | **−19 %** (gs is 2.58; still trailing) |
| B10   | 3.13 | 2.50 | **−20 %** (gs is 2.75; ahead) |

That would put sli3 ahead of gs on B1/B2/B3/B5/B7/B10, tied on
B7, still trailing on B2b (within 10 %) and B9 (recursion is
inherently dispatch-heavy and gs's tail-recursion + 2-byte packed
slots are intrinsic advantages).

## What changed: the old per-slice descriptions below

The per-slice descriptions in the old plan (Slices 4-9, retained
verbatim below this section) are now **superseded** by this
re-costing. They remain for historical context; the actionable
plan is:

- Skip Slices 4, 5, 6, 7 as separately committable steps. They
  ship as a single bundle, **Axis I bundle**, when the work is
  done — see the new section "Axis I bundle" below.
- Slice 8 is unchanged in spirit but the cost model is now
  current.
- Slice 11 is new.

## Axis I bundle (replaces old Slices 4-7) — ✅ DONE 2026-05-12

Goal: one bundled change that adds `current_op_`, drops the
function-Token push at the dispatcher, drops the per-operator
`EStack().pop()`, and inverts the contract from "operator self-pops
on the way out" to "dispatcher pre-pops on the way in".

**Actual approach (what shipped):**

1. ✅ Add `SLIInterpreter::current_op_` field + forward
   declaration. `get_current_name` consults `current_op_`
   first, falls through to e-stack read. **Commit `27d5380`.**
2. ✅ Dispatcher: at all 9 fn-dispatch sites, set `current_op_ = fn`
   before the call, clear after. **Commit `86d2d3a`.** Iter
   cases push a `nulltype` sentinel for old-ABI ops so their
   self-pop doesn't corrupt the iter frame.
3. ✅ Per-file ABI conversion (commits `d3766b8` through
   `373d24f`):
   - 3a — dual-ABI dispatcher skeleton (post-check pops if
     new-ABI op left its slot on top). `46ef896`.
   - 3b — `sli_stack.cpp` (~16 ops). `d3766b8` / `ea9f9d8`.
   - 3c — `sli_math.cpp` (~105 ops). `f88f18c`.
   - 3d — `sli_container_ops.cpp` (~73 ops). `f89ffbb`.
   - 3e — `sli_control.cpp` trailing-pop ops (~17 ops).
     `25f78c1`.
   - 3f — 6 smaller files (typecheck, io, startup, array_module,
     state_ops, readline). `373d24f`.
4. ✅ **Contract revision** (commit `151e5e5`). The post-check
   approach was replaced by a **pre-pop contract**: the
   dispatcher pops the fn slot BEFORE `fn->execute` for
   new-ABI ops. `raiseerror(Name)` and `raiseerror(exception&)`
   conditionally skip their own pop based on
   `current_op_->uses_new_abi()`. `FunctionType::execute`
   (plain mode) mirrors the pre-pop. This enabled converting
   ~30 more ops (conditionals, switch family, cv* dispatcher
   arms, savestate/restorestate) that the post-check couldn't
   handle. **Step-4 audit caught four latent bugs** that the
   post-check fallback had been masking (pwrite_fn → `==` was
   broken; arrayload_fn, getmax_fn, getmin_fn). Regression
   test: `tests/test_dispatch_abi.cpp`.

**Bench gate**: B2b ≤ 1.80 s, B7 ≤ 3.10 s. **Hit**: B2b 1.79 s,
B7 2.10 s (best-of-five at `151e5e5`). B1 down to 0.95 s (was
1.33 at `stage9-complete`).

**Status of the dual-ABI flag**: `new_abi_` remains as an opt-in
flag. ~250 of ~316 sites are new-ABI; the rest (Map family iter
setups, ExecFunction, ExitFunction, StopFunction,
CloseinputFunction, iparse / iparsestdin) stay old by design —
they manage their own frame in non-trivial ways. Flipping the
default and dropping the flag is a future cleanup with no bench
impact.

## Slice 11 — pvalue cache on Name (P1 from next_steps.md)

**Goal:** make name lookup one pointer indirection instead of a
list walk through the dictionary stack. Targets the 10-15 %
`DictionaryStack::lookup` cost on B5/B7/B9/B10.

`Name::values_` (a `std::vector<Token*>` parallel to
`Name::table_`) records the current dict-stack slot for each
interned Name. The slot pointer is updated when names are
defined / undefined / when dicts are pushed / popped:

- `DictionaryStack::insert(n, t)`: write the just-allocated
  dict slot's `Token*` to `Name::values_[n.toIndex()]`. If the
  slot is already set (the name is defined in another dict on
  the stack), flip it to a "multi" sentinel.
- `Dictionary::erase(n)`: walk the dict stack and re-find the
  next-highest definition, or clear to nullptr.
- `Dictionary::clear()` / dict-stack `pop()`: invalidate slots
  pointing into the popped dict.

`DictionaryStack::lookup(n)` becomes:

```cpp
Token& DictionaryStack::lookup(Name const& n) {
    Token* cached = Name::values_[n.toIndex()];
    if (cached == kMulti) /* fall through to dict-stack walk */;
    else if (cached) return *cached;
    /* slow path: list walk + populate cache */
}
```

In the common single-definition case (e.g. `add` is in
`systemdict` only), the cache hit is one array load + one
dereference. The current code walks the dict stack via
`std::list<Dictionary*>::iterator`, hashes the name, and
returns. The 10-15 % cost on B5/B7/B10 should largely vanish.

**Risk:** cache coherence at every dict-stack mutation. The
sli_dictstack.h `DICTSTACK_CACHE` already does something
similar (cache `Token*` keyed by Name handle per dict-stack
frame), so much of the infrastructure exists. The new cache is
just a slot-pointer table on `Name` itself, indexed by handle.

**Test:** add tests/test_pvalue_cache.cpp exercising:
- single-definition lookup (cache hit)
- multi-definition lookup (cache miss → fall through)
- begin / end (cache invalidate + restore)
- undef (cache clear)
- def followed by undef + def of same name in another dict (state
  transition through multi)

**Bench gate:** B5 ≤ 1.40 s, B7 ≤ 3.00 s, B9 ≤ 3.50 s, B10 ≤
2.80 s. (Combined with the new ABI from Axis I bundle, these
are 10-15 % below current.) If shipping standalone (before
Axis I bundle), bench gate is more modest: B5 ≤ 1.55 s, B7 ≤
3.15 s, B10 ≤ 2.95 s.

**Independence:** Slice 11 is independent of Axes I, II, III.
Could land first (recommended) or in parallel with Axis I bundle
prep.

---

**Goal:** capture today's numbers as the reference point.

1. `cmake --build build -j --target sli3` from a clean
   `build/`.
2. `bench/run.sh > /tmp/bench-baseline.txt` (5 runs each;
   captures B1/B2/B2b/B3/B5/B6 family).
3. `ctest --test-dir build` clean.
4. Profile B2b with Instruments (`xcrun xctrace record
   --template "Time Profiler" --launch -- ./build/sli3 <
   bench/sli/B2b_add_pop_bound.sli`) — at minimum, identify
   the top 10 functions by self-time. Compare against this
   list after each slice to spot unintended regressions.

**Commit:** none — just a record. Append to a working notes
file (`/tmp/axis1-notes.md` or similar).

**Gate:** none.

---

## Slice 1 — Add `current_op_` field, refactor raiseerror

**Status (2026-05-11): deferred. Merged into Slice 4.**

Attempted as specified below; reverted. Measured cost: **+4.8% on
B2b** (1.89 s → 1.98 s best-of-five). Root cause: the wrap adds 2
member-field stores per op dispatch (`current_op_ = fn` before
`fn->execute(this)`, clear after). For B2b that's ~0.5 ns × 200M
dispatches = ~0.1 s, matching the observed regression.

The ±2% bench gate is hard. Slice 1 as a standalone refactor is
not zero-cost; the cost-of-correctness can only be offset by
**removing** something on the same path. That removal is the
e-stack push in Slice 4. So `current_op_` appears at Slice 4 where
the new field is paired with the removed push:

```cpp
// Slice 4: replace the function-Token push with current_op_ write
// + sentinel write. Net per-op cost is the sentinel + 2 stores
// vs. today's full Token push (4 writes + refcount-aware add).
current_op_ = fn;
execution_stack_.top() = dispatch_sentinel;
fn->execute(this);             // op's self-pop pops the sentinel
current_op_ = nullptr;
```

`get_current_name` is updated to consult `current_op_` first at
the same slice. Slice 2's nested-call audit applies unchanged
(it touches direct-call sites in `src/builtins/`, not the
dispatcher).

The text below is the original Slice 1 spec, preserved for
reference.

**Goal:** decouple `raiseerror` from reading the e-stack top.
This is a pure refactor: behavior unchanged.

`SLIInterpreter` gains a single field:

```cpp
private:
    SLIFunction const* current_op_ = nullptr;
```

The dispatcher's existing inline `functiontype` arms and the
standalone `case sli3::functiontype:` set this field before
calling `fn->execute(this)` and clear it after. The push of
the function Token onto the e-stack continues unchanged
(we're not yet removing it).

```cpp
case sli3::functiontype: {
    SLIFunction* fn = execution_stack_.top().data_.func_val;
    execution_stack_.push(execution_stack_.top());  // unchanged
    SLIFunction const* prev = current_op_;
    current_op_ = fn;
    fn->execute(this);
    current_op_ = prev;
    break;
}
```

Same edit in each iter case's `functiontype` arm.

`get_current_name` (`sli_interpreter.cpp:363`) consults
`current_op_` first; falls through to the existing e-stack
read if null:

```cpp
Name SLIInterpreter::get_current_name() const {
    if (current_op_)
        return current_op_->get_name();
    if (execution_stack_.top().is_of_type(sli3::functiontype))
        return execution_stack_.top().data_.func_val->get_name();
    return interpreter_name;
}
```

Same for `raiseerror(Name err)` (`:372`): use `current_op_` if
set, else the e-stack top.

**Files touched:** `sli_interpreter.h`, `sli_interpreter.cpp`.
~30 lines.

**Tests:** `ctest --test-dir build` clean; in particular
`test_errors`, `test_errors_dispatch`, `test_dispatch_parity`,
`test_sli_eval`.

**Bench gate:** B2b unchanged ± 2 % (5 runs).

**Risk:** very low. The new field is set/cleared in 5-10
places; reads in 2.

---

## Slice 2 — Save/restore `current_op_` across nested op calls

**Goal:** make nested operator dispatch (op A's body calling
op B via `fn->execute(this)`) preserve attribution.

After slice 1, `current_op_` is overwritten by any nested op
call. The default for nested operators is to attribute errors
to the inner op (matches gs). For sites that prefer outer-op
attribution (rare), they can save/restore explicitly:

```cpp
// In op A's execute body, calling op B:
SLIFunction const* saved = i->current_op_;
i->baselookup(Name("op-B")).data_.func_val->execute(i);
i->current_op_ = saved;
```

But this is the *opposite* of what we want — we want B to be
the attributed op. The dispatcher already sets `current_op_`
before each call, so this is automatic. Slice 2's actual job
is to **audit the ~20 direct-call sites** and decide which
ones should preserve the outer op vs let the inner op
attribute.

Inventory pass: `grep -rn "data_.func_val->execute\|baselookup.*execute" src/builtins/`.
For each site, document in the patch which attribution is
correct.

**Files touched:** `src/builtins/sli_control.cpp` (`ifelse`,
`def`-dispatch), `src/builtins/sli_typecheck.cpp` (trie
forwarding), `src/builtins/sli_container_ops.cpp` (cva, get,
put dispatchers). Estimated 20-30 sites.

**Tests:** `test_errors_dispatch` must keep passing. Add a
new test in `test_errors.cpp` that exercises a nested-op
error and asserts which op is attributed in
`$errordict /commandname`.

**Bench gate:** B2b unchanged.

**Risk:** low. Audit-and-document slice, very few real edits.

---

## Slice 3 — Lift dispatcher locals into registers

**Goal:** reduce the per-token overhead of re-picking `proc`,
`pos`, `iosp`, `iesp` from the e-stack inside iter cases.
Pure performance refactor; no semantic change.

Currently the dispatcher loop reads stack pointers via member
accesses (`operand_stack_.p`, `execution_stack_.p`) and the
iter cases pick `proc` from `execution_stack_.pick(2)` and
re-load on every `goto start_repeat`. Lift these to locals at
the top of `execute_dispatch_`:

```cpp
int SLIInterpreter::execute_dispatch_(size_t exitlevel) {
    register Token*    iosp = &operand_stack_.top();
    register Token*    iesp = &execution_stack_.top();
    register size_t    local_cycles = cycle_count_;
    SLIType*           iiterate_t = get_type(sli3::iiteratetype);
    SLIType*           proc_type  = get_type(sli3::proceduretype);
    ...
}
```

Re-sync to the member fields before any C operator call:

```cpp
case sli3::functiontype: {
    operand_stack_.p = iosp;  // sync for the operator
    execution_stack_.p = iesp;
    ...
    fn->execute(this);
    iosp = &operand_stack_.top();   // pull back
    iesp = &execution_stack_.top();
    break;
}
```

In the four iter cases, lift `proc` / `pos` into locals at
the top of the case (already done partially; finish the job
so the loop tail doesn't re-pick).

This is delicate: the dispatcher must keep
`operand_stack_.p` / `execution_stack_.p` in sync at any
point where (a) a C operator can read them, (b) an exception
might unwind, (c) the operator can resize the underlying
vector. Re-sync points:

- Before each `fn->execute(this)` call.
- Before each `throw` (the `raiseerror` path).
- After each return from `fn->execute(this)`.
- At loop-exit (write back to members).

**Files touched:** `sli_interpreter.cpp` only.

**Tests:** `ctest` clean. In particular, run
`test_round_trip`, `test_dispatch_parity` — these exercise
the dispatcher under heavy stack pressure.

**Bench gate:** B1 ≤ 1.32 s, B2b ≤ 1.86 s (expecting 0.05-0.1
s improvement just from register locals).

**Risk:** moderate. The aliasing between `iosp` and
`operand_stack_.p` is the main hazard. Use `-Wall -Wextra`
and ASAN through every test.

**Revert criterion:** if any test fails or B2b regresses,
revert. The other slices don't depend on slice 3's locals
being in registers; they just benefit from them.

---

## Slice 4 — Drop the function-Token e-stack push + add `current_op_`

> **2026-05-12: SUPERSEDED by the Axis I bundle (see Re-costing
> section above).** This slice's bench gate of "+5 % B2b
> recovered at Slice 6" is unmet by the current cost model
> because the sentinel doesn't save the refcount-virtual-call
> it was predicted to save (`Token::Token` already short-
> circuits via `needs_refcount()`). The work merges into the
> Axis I bundle which only commits a passing bench at the end
> of the whole sequence (not per-slice). Description retained
> below for context.

**Goal:** the dispatcher no longer pushes the function Token
before calling `fn->execute`. This means the function Token
is *not* on the e-stack during the operator's execution. At the
same time, introduce `current_op_` (originally Slice 1, deferred
to here so the two changes net out to ~0 cost — see Slice 1).

At this slice, **operators still self-pop** — we cannot drop
that yet without removing it in 316 places. So we need a
shim: the dispatcher push a *sentinel* Token before calling
the operator, so the operator's self-pop pops the sentinel
instead of corrupting the dispatcher's state. Concurrently,
`current_op_` records the active operator so `raiseerror`
and `get_current_name` don't need to read the e-stack.

**Surface added at this slice (from Slice 1):**
- `SLIInterpreter::current_op_` field, default nullptr.
- `class SLIFunction;` forward declaration in
  `sli_interpreter.h`.
- `get_current_name()` consults `current_op_` first, falls
  through to the e-stack read.
- `raiseerror(Name)` and `raiseerror(std::exception&)` route
  the caller name through `get_current_name`.
- The dispatcher writes `current_op_ = fn; ... = nullptr;`
  around every `fn->execute(this)` call (9 sites).

```cpp
// Single global sentinel, not refcounted.
static Token const dispatch_sentinel{
    Token::pack_type(types_[sli3::nulltype])
};

case sli3::functiontype: {
    SLIFunction* fn = execution_stack_.top().data_.func_val;
    current_op_ = fn;
    execution_stack_.top() = dispatch_sentinel;   // overwrite with sentinel
    fn->execute(this);    // op's self-pop pops the sentinel
    current_op_ = nullptr;
    break;
}
```

`get_current_name` and `raiseerror` already use `current_op_`
(slice 1). The e-stack now contains a sentinel where the
function Token used to be — but for the duration of the
op's execution only.

This is a holding pattern. Slice 5 removes the sentinel and
the operator self-pops simultaneously, file by file.

**Files touched:** `sli_interpreter.cpp` (the four iter
cases' `functiontype` arms + the standalone arm + trie arm).

**Tests:** `ctest` clean. Add an explicit test that
`current_op_->get_name()` matches what was previously read
from `EStack().top()` for each error type in
`test_errors_dispatch`.

**Bench gate:** B2b ≤ 1.85 s (expecting ~0.05 s; we removed
the push of the actual function Token, but added the
sentinel write — net saves ~1 ns/op via skipping the
refcount-check virtual call on the function Token).

**Risk:** moderate. The sentinel must be a Token whose
destructor is harmless (non-refcounted). `nulltype` is the
right choice — `NullType::add_reference` / `remove_reference`
are no-ops.

**Revert criterion:** if any test fails or B2b regresses.

---

## Slice 5 — Drop operator self-pop, batch 1: `sli_stack.cpp`

> **2026-05-12: SUPERSEDED by the Axis I bundle.** Per-file commits
> happen as steps 3.a-3.h inside the bundle, but the bench gate is
> at the end of the bundle (not per file). Description retained
> below for the per-file mechanics.

**Goal:** start the ~316-site ABI conversion. Smallest file
first (~10 sites). The dispatcher's sentinel from slice 4 is
removed for ops in this file; the dispatcher no longer
writes the sentinel before calling these specific ops.

This means we need to **distinguish "new-ABI ops" from
"old-ABI ops"** in the dispatcher. One way: add a flag on
`SLIFunction`:

```cpp
class SLIFunction {
    bool new_abi_ = false;
public:
    bool uses_new_abi() const { return new_abi_; }
    ...
};
```

Set `new_abi_ = true` in the constructor of every operator
class converted to the new ABI. The dispatcher:

```cpp
case sli3::functiontype: {
    SLIFunction* fn = execution_stack_.top().data_.func_val;
    current_op_ = fn;
    if (fn->uses_new_abi()) {
        execution_stack_.pop();          // pop the function Token
        fn->execute(this);               // op does NOT self-pop
    } else {
        execution_stack_.top() = dispatch_sentinel;
        fn->execute(this);               // op self-pops the sentinel
    }
    current_op_ = nullptr;
    break;
}
```

Process `sli_stack.cpp` (~10 ops: `pop`, `dup`, `exch`,
`index`, `count`, `clear`, `rot`, `rollu`, `rolld`, `roll`):

1. Remove the trailing `i->EStack().pop()` from each
   `execute()`.
2. Set `new_abi_ = true` in each operator's constructor.
3. Rebuild.
4. `ctest`.

**Files touched:** `src/builtins/sli_stack.cpp`,
`src/interpreter/sli_function.h`, `sli_interpreter.cpp` (the
two-armed dispatch).

**Tests:** `ctest` clean. The stack-op tests
(`test_stack_ops_negatives.cpp`) directly cover these ops.

**Bench gate:** B1 (1 pop × 100 M) should drop noticeably —
`pop` is the only op in B1, and after slice 5 it skips the
push and self-pop. Expected B1: 1.33 s → ~1.25 s.

**Risk:** low per-site. The mechanical change is "delete one
line and set a flag." Catch a missed pop with the dispatcher
assertion (slice 8 adds a hard one; for now, manual
inspection + tests).

---

## Slice 6 — Drop operator self-pop, remaining batches

> **2026-05-12: SUPERSEDED by the Axis I bundle.** See Slice 5
> note. The per-file order in this section is the recommended
> order inside the Axis I bundle.

**Goal:** convert the remaining ~306 sites in chunks. One
commit per source file:

1. `sli_math.cpp` (~40 ops: `add`, `sub`, `mul`, `div`,
   `mod`, comparisons, math functions). Largest single file;
   biggest expected B2b gain.
2. `sli_typecheck.cpp` (~50 ops).
3. `sli_control.cpp` (~40 ops, including `if`, `ifelse`,
   `def`-dispatch, `for`, `repeat`, `loop`, `stop`,
   `stopped`, `raiseerror`).
4. `sli_container_ops.cpp` (~80 ops).
5. `sli_array_module.cpp` (~30 ops).
6. `sli_io_ops.cpp` (~30 ops).
7. `sli_startup.cpp` (~30 ops).
8. `sli_builtins.cpp` (~10 ops: `iiterate`, `iloop`,
   `irepeat`, `ifor`, `iforallarray`,
   `iforallindexedarray/string`, `iforallstring`,
   `arraycreate`, `dictconstruct`). **These are tricky**:
   the iter ops push state onto the e-stack as part of
   their work; the dispatcher's iter cases consume that
   state. Slice 6's last commit, handled with care.

For each file:

1. Remove every trailing `i->EStack().pop()`.
2. Set `new_abi_ = true` in every ctor.
3. Rebuild + `ctest`.
4. Bench (`bench/run.sh`). Record results.
5. Commit.

**Files touched:** ~8 commits, each one source file.

**Tests:** every commit must pass `ctest --test-dir build`.

**Bench gates:**
- After `sli_math.cpp`: B2 drops noticeably (every B2 iter has
  an `add`).
- After `sli_control.cpp`: B2b drops noticeably (every B2b
  iter has the iter-frame setup via repeat).
- After all files: B2b ≤ 1.65 s (matches "Axis I A1
  variant" prediction).

**Risk:** moderate. The mechanical change is mostly safe but
there are subtleties:

- **Operators that pop more than themselves.** A few
  operators end with `i->pop(N); i->EStack().pop()` where
  the EStack pop balances *something the operator pushed
  earlier in its body* (e.g. it pushed a temporary onto
  estack and now pops it). These are not "self-pops" and
  must stay. Inventory pre-pass: `grep -n
  "EStack().push" src/builtins/*.cpp` — every site that
  pushes needs to be matched against its pop.
- **Operators that don't end in self-pop**: a few use
  `return` before the pop, or have multiple return paths.
  Code review per file.
- **Operators that push a continuation frame**: `repeat`,
  `for`, `loop`, `forall`. After their setup, control
  returns from `execute` with the new frame on top; the
  dispatcher then enters the body. With new ABI, these
  operators don't self-pop but they leave a *real frame* on
  top, not a sentinel. The dispatcher needs to recognise
  that the e-stack top is now the iter-type marker, not the
  function Token. Verify the existing
  `irepeattype`/`ifortype` dispatch path works after these
  ops return.

**Revert criterion:** if `ctest` fails or B2b regresses on
any commit, revert that commit and investigate.

---

## Slice 7 — Remove the dual-ABI shim; require new ABI

> **2026-05-12: SUPERSEDED by the Axis I bundle.** The dual-ABI
> shim is set up in step 2 and removed in step 4 of the bundle.
> The description below for what removing the shim entails is
> still accurate.

**Goal:** after slice 6, every operator in `src/builtins/` is
on the new ABI. Drop the `uses_new_abi()` check and the
sentinel path from the dispatcher.

```cpp
case sli3::functiontype: {
    SLIFunction* fn = execution_stack_.top().data_.func_val;
    execution_stack_.pop();
    current_op_ = fn;
    fn->execute(this);
    current_op_ = nullptr;
    break;
}
```

Drop `SLIFunction::new_abi_` (now always true) and
`uses_new_abi()`. Add an assertion in the dispatcher's
`default:` arm:

```cpp
default:
    // Catch ops that don't yet implement the new ABI.
    assert(false && "operator not converted to new ABI");
    execution_stack_.top().execute();   // fallback
```

**Files touched:** `sli_function.h`, `sli_interpreter.cpp`.

**Tests:** `ctest` clean. Whole suite.

**Bench gate:** B2b ≤ 1.65 s (unchanged from slice 6 end;
this is a cleanup).

**Risk:** low.

---

## Slice 8 — Inline the body walk into the main dispatcher

> **2026-05-12: cost model updated.** Original prediction was
> −0.10 to −0.15 s on B2b. Revised: B2b drops to ~1.50-1.55 s
> after Axis I bundle + slice 8, which is −15 to −20 % of the
> current 1.90 s. Risk and approach unchanged; the structural
> change is the same.

**Goal:** the biggest single structural win. The dispatcher
walks procedure bodies inline (gs's `top:` topology) instead
of via the iter-case re-dispatch round-trip.

This is where the design gets aggressive. The dispatcher
loop becomes:

```cpp
int SLIInterpreter::execute_dispatch_(size_t exitlevel) {
    register Token const* iref = nullptr;
    register long icount = 0;
    register Token*       iosp = &operand_stack_.top() + 1;
    register Token*       iesp = &execution_stack_.top() + 1;

    try {
      while (execution_stack_.load() > exitlevel) {
        if (iref == nullptr) {
            // Not inside a body. Look at the e-stack top.
            switch (iesp[-1].tag()) {
                case sli3::iiteratetype:
                case sli3::irepeattype:
                case sli3::ifortype:
                case sli3::iforalltype:
                    enter_iter_body(/* in/out */ iref, icount, iesp);
                    if (iref == nullptr) {
                        // body exhausted; iter epilogue ran; loop
                        continue;
                    }
                    break;
                // ... other top-level cases: parser, functiontype-at-REPL ...
            }
        }
      top:
        switch (iref->tag()) {
            case sli3::integertype: case sli3::doubletype:
            case sli3::booltype: case sli3::literaltype:
            case sli3::marktype: case sli3::stringtype:
            case sli3::arraytype: case sli3::dictionarytype:
                operand_stack_.push(*iref);
                goto advance;
            case sli3::nametype: {
                Token const& resolved = lookup(iref->data_.long_val);
                if (resolved.tag() == sli3::functiontype) {
                    SLIFunction* fn = resolved.data_.func_val;
                    current_op_ = fn;
                    store_state(iref, icount);  // before fn may unwind
                    fn->execute(this);
                    current_op_ = nullptr;
                    if (estack_changed_since_store_state()) {
                        // op pushed new frames; sync and re-enter
                        iref = nullptr; continue;
                    }
                    load_state(iref, icount);
                    goto advance;
                }
                if (resolved.is_executable()) {
                    store_state(iref, icount);
                    execution_stack_.push(resolved);
                    iref = nullptr; continue;
                }
                operand_stack_.push(resolved);
                goto advance;
            }
            case sli3::functiontype: {
                SLIFunction* fn = iref->data_.func_val;
                current_op_ = fn;
                store_state(iref, icount);
                fn->execute(this);
                current_op_ = nullptr;
                if (estack_changed_since_store_state()) {
                    iref = nullptr; continue;
                }
                load_state(iref, icount);
                goto advance;
            }
            case sli3::proceduretype: {
                store_state(iref, icount);
                execution_stack_.push(iref->data_.array_val);  // enter nested
                iref = nullptr; continue;
            }
            // ... trie, etc.
        }
      advance:
        if (--icount <= 0) {
            // body finished — pop the frame and check parent
            iesp = pop_iter_frame();
            iref = nullptr;
            continue;
        }
        ++iref;
        goto top;
      }
    } catch (std::exception& e) {
        raiseerror(e);
    }
    ...
}
```

`enter_iter_body` (helper): given an iter-type frame on top
of estack, set `iref` to point at the next ref in the body
and `icount` to the count remaining. If body exhausted,
run the iter epilogue (dec counter / reset pos / pop frame)
and return with `iref == nullptr` so the outer loop
re-examines the e-stack.

`store_state` (helper): write the current `iref` + `icount`
back into the active iter frame on the e-stack (the frame's
`pos` field). Needed before any op call that might `throw`.

`load_state` (helper): re-read from the frame after an op
returns (in case `pos` was modified — usually not).

`estack_changed_since_store_state` (helper): compare current
`iesp` to a snapshot taken at `store_state` time. If grew,
the op pushed new frames; we need to re-enter via outer loop.

**Files touched:** `sli_interpreter.cpp` (the dispatcher),
plus the iter operators (`sli_builtins.cpp:175-330`) need to
keep producing the e-stack frame shape the dispatcher
expects. The iter ops' `execute` bodies become much smaller
(just frame setup, no body walking).

**Tests:** full `ctest` clean. Add a new test
`test_dispatch_topology.cpp` that exercises:
- Nested procedures (proc calling proc calling proc).
- Mixed iter types nested (`for { repeat { ... } }`).
- Stop unwind through several iter frames.
- Error raised inside a 5-deep proc nesting.

**Bench gate:** B2b ≤ 1.55 s (the target). B1 ≤ 1.30 s. B3
≤ 1.85 s. B5 ≤ 16.0 s.

**Risk:** high. This is the riskiest slice. The semantics
must be preserved exactly, but the dispatcher's structure
is fundamentally different.

Mitigations:

- Land slice 8 behind a build flag (`-DSLI3_INLINE_BODY_WALK
  =ON`) for the first PR. CI runs both modes. Once the new
  mode is stable for a week, flip default; later, delete the
  old path.
- Keep `execute_` (the non-debug, non-dispatch mode) and
  `execute_debug_` unchanged; they're only used for testing
  and edge cases. Restructure only `execute_dispatch_`.
- Add a comparison test: pick 10 SLI scripts; run each
  through both old and new dispatchers; assert the operand
  stack contents and `$errordict` are identical at the end.

---

## Slice 9 — Cleanup: continuation-op style for iter ops (optional)

**Goal:** convert iter operators to gs-style continuation
operators. Replaces the iter-type markers
(`iiteratetype`/`irepeattype`/...) with C operator
continuations (`iiterate_continue_fn`/`irepeat_continue_fn`/
...). The dispatcher's main loop no longer needs the four
iter-type marker cases at all.

This is optional. The win is marginal (one fewer special
case in the dispatcher); the cost is touching every test
and operator that depends on the existing iter-frame shape.

Defer to a later cleanup PR if it's worth doing at all.

**Bench gate:** B2b unchanged from slice 8 (this is shape,
not perf).

---

## Slice 10 — `savestate` / `restorestate` operators

**Goal:** add user-visible operators that snapshot and restore
the interpreter state. The State invariants section above
makes this implementable; this slice exposes it to SLI code.

This is the first consumer that *requires* the invariants to
hold. It's also a useful debugging / checkpoint feature in its
own right and provides a comprehensive regression test for the
invariants.

### Surface

Two operators registered in `src/builtins/sli_state_ops.cpp`:

```sli
% Capture the current operand stack, execution stack, and
% dict stack into a file. The savestate op itself is not
% recorded; the snapshot is the state as if savestate had
% just returned.
%
% (filename) savestate -> -

% Replace the current operand / execution / dict stacks
% with the snapshot's contents. Object identity within the
% snapshot is preserved; identity with pre-restore objects
% is not.
%
% (filename) restorestate -> -
```

### Snapshot contents

Three sub-streams in the binary format:

1. **Operand stack:** array of Tokens, deep-serialized.
2. **Execution stack:** array of Tokens, deep-serialized,
   with the savestate operator's own frame already removed
   (the snapshot is taken at the logical boundary *after*
   savestate returns).
3. **Dict stack:** array of dict references. Permanent dicts
   (`systemdict`, `errordict`, `userdict`, `statusdict`) are
   identified by their well-known names and re-bound to the
   current interpreter's instances on restore. User-defined
   dicts are restored by-value (new dict objects with the
   saved contents).

The format reuses the existing `Writer` / `Reader` protocol in
`src/serialize/`. The `Writer`'s `intern_object` table
deduplicates within the snapshot, so dicts and procedures
referenced multiple times share identity on the wire.

### Per-type serialize coverage required

The existing `src/serialize/` covers Integer, Double, Bool,
Name/Literal/Symbol/Mark, String, TokenArray (arrays /
procedures / litprocedures), and streams (placeholders with
warnings). **Three gaps must be filled before savestate works:**

| Type | Serialize | Deserialize |
|---|---|---|
| `FunctionType` | write `SLIFunction::get_name()` | look up name in dictionary stack; assign `data_.func_val` from the resolved Token |
| `TrieType` | write `TypeNode::get_name()` | look up name in dictionary stack; assign `data_.trie_val` |
| `DictionaryType` | intern → write contents (`name, token` pairs); or write a sentinel for permanent dicts (systemdict / etc.) | matching read; identity-preserving lookup for permanent dicts |

These overrides land alongside the operators in this slice.
They're independently useful — every type having a serialize
override is part of the project's documented contract
(CLAUDE.md: "Every `SLIType` subclass must override
`serialize` / `deserialize`").

### Implementation sketch

```cpp
class SaveStateFunction : public SLIFunction {
public:
    void execute(SLIInterpreter* i) const override {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string filename = i->top().data_.string_val->str();
        i->pop();
        // Pre-Axis-I: self-pop first so the e-stack snapshot
        // reflects "savestate has returned". Post-Axis-I,
        // the dispatcher does this; the line goes away with
        // every other self-pop in slice 6.
        i->EStack().pop();

        std::ofstream out(filename, std::ios::binary);
        if (!out) { i->raiseerror(i->BadIOError); return; }
        BinaryWriter w(out);
        w.write_header();

        write_token_stack(i->OStack(), w);
        write_token_stack(i->EStack(), w);
        write_dict_stack (i->DStack(), w);
    }
};

class RestoreStateFunction : public SLIFunction {
public:
    void execute(SLIInterpreter* i) const override {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string filename = i->top().data_.string_val->str();
        i->pop();
        i->EStack().pop();   /* self-pop; see savestate */

        std::ifstream in(filename, std::ios::binary);
        if (!in) { i->raiseerror(i->BadIOError); return; }
        BinaryReader r(in);
        r.read_header();

        /* Read into temporaries first; only swap once all reads succeed. */
        TokenStack new_ostack, new_estack;
        DictionaryStack new_dstack;
        read_token_stack(new_ostack, r, *i);
        read_token_stack(new_estack, r, *i);
        read_dict_stack (new_dstack, r, *i);

        /* Atomic replace. */
        i->OStack().swap(new_ostack);
        i->EStack().swap(new_estack);
        i->DStack().swap(new_dstack);
    }
};
```

### Interaction with Axis I

- Pre-Axis-I (today): both ops self-pop before serializing,
  so the e-stack snapshot excludes the op itself. Works.
- Post-Slice-4: the dispatcher writes a sentinel into the
  op's slot before calling; the op's `EStack().pop()` pops
  the sentinel. Works.
- Post-Slice-7: the op no longer self-pops; the dispatcher
  has already popped the op's Token by the time `execute()`
  runs. Works.
- Post-Slice-8: the dispatcher's register-held state is
  synced to the iter frame's `pos` field *before* the op
  call (per Invariant 2), so the e-stack accurately reflects
  the resume position. Works.

The implementation is forward-compatible: it can land *before*
Axis I starts (the self-pop pattern matches every other op
today). It then gets converted along with the rest in slice 6.

### Limitations to document

- **Object identity across save/restore is not preserved.**
  If `dict_a` is mutated after save and then restorestate
  fires, the dict-stack's references to `dict_a` may point at
  the snapshot's separate copy. The original `dict_a` becomes
  orphaned. Same for procedures, strings, and other ref types.
- **Permanent dicts are re-bound by name.** `systemdict`,
  `errordict`, `userdict`, `statusdict` references in the
  snapshot resolve to the current interpreter's instances on
  restore. This is the *right* behavior: built-in operator
  bindings remain valid. The cost is that user mutations to
  these dicts between save and restore are not rolled back.
- **Open streams are not snapshottable.** Existing convention:
  `IstreamType::serialize` / `OstreamType::serialize` emit a
  warning and produce closed-stream Tokens on load. The
  caller must reopen any files needed after restore.
- **C-side state outside the three stacks is not snapshotted.**
  `cycle_count_`, `call_depth_`, `verbosity_level_`,
  `count_calls_` etc. stay at their current values. The
  contract: the snapshot is *PostScript-level program state*,
  not interpreter internals.

### Tests

Add `tests/test_state_ops.cpp`:

1. **Round-trip ostack only.** `1 2 3 (/tmp/x) savestate`
   followed by `(/tmp/x) restorestate` produces the same ostack.
2. **Round-trip mid-loop.** Set up a `for` loop, savestate
   inside the body at iteration N, restorestate in a fresh
   block, verify the loop resumes at iteration N+1 (Invariant 2
   exerciser).
3. **Bind survives.** `/p { 1 add } bind def 5 (/tmp/x)
   savestate ...` then restorestate, then `p` — verifies that
   the procedure's bound operator references round-trip
   (FunctionType serialize gap).
4. **Dict-stack restoration.** `<< /k 1 >> begin` then
   savestate inside, restorestate from outside any begin —
   verify the dict stack has the snapshot's local dict on top.
5. **Permanent dict identity.** After restorestate, verify
   `systemdict /add known` still finds the same operator
   (FunctionType resolved via name).

### Bench expectations

Not a perf slice. Savestate/restorestate run once per
checkpoint, not in a hot loop. Bench gate: no regression on
any B-bench.

### Files touched

- `src/serialize/sli_serialize.{h,cpp}` — add `write_token_stack`,
  `read_token_stack`, `write_dict_stack`, `read_dict_stack`
  helpers.
- `src/types/sli_functiontype.cpp` — add serialize / deserialize.
- `src/trie/sli_trietype.h` (or new .cpp) — add serialize /
  deserialize.
- `src/types/sli_dicttype.cpp` (new file or append to .h
  inline implementations) — add serialize / deserialize.
- `src/builtins/sli_state_ops.{h,cpp}` — new file.
- `src/interpreter/sli_interpreter.cpp` — register operators
  in `init_internal_functions`.
- `tests/test_state_ops.cpp` — new test.
- `CMakeLists.txt` — add source + test.

### Position in the slice order

Slice 10 can land **before, during, or after** slices 1–9. It
depends on no Axis I machinery; it benefits from the State
invariants document but doesn't require any code change to
the dispatcher. Suggested placement:

- **Before slice 1:** lands the per-type serialize coverage and
  the operator surface. Subsequent Axis I slices (5–8) treat
  it like any other operator pair when converting to the new
  ABI.
- **Between slice 7 and slice 8:** lands after the ABI is
  clean, before the structural dispatcher rewrite. Provides
  a regression test (mid-loop save/restore round-trip) for
  slice 8.

Default recommendation: **land before slice 1.** The serialize
gap-filling is independently useful, the operators give
slice 8 a regression test, and the change has its own
non-Axis-I review surface.

---

## Risk dashboard

**2026-05-12: post-Axis-I-bundle update.**

| Slice | Surface | Risk | Bench delta (measured / expected) | Notes |
|---|---|---|---|---|
| 0 | none | — | baseline | ✓ done |
| 1 | dispatcher + raiseerror | low | +4.8 % B2b — reverted | merged into Axis I bundle |
| 2 | ~51 nested-call sites audit | low | 0 | ✓ done (`doc/axis1_slice2_audit.md`) |
| 3 | pos pointer hoist in iter cases | medium | **−15.6 % B1, −6.9 % B2b, −9.0 % B3** | ✓ done (commit `a9c3d25`) |
| Axis I bundle (4-7) | dispatcher + ~250/316 builtin sites | high | **measured: B1 −29 %, B3 −28 %, B5 −91 % (also from Slice 11), B7 −4 %; B2b parity** | ✓ done (commits `46ef896`..`151e5e5`). Step 4 pre-pop contract revision + audit fixes (pwrite_fn / arrayload_fn / getmax_fn / getmin_fn). Regression test: `tests/test_dispatch_abi.cpp`. |
| 8 | dispatcher rewrite (inline body walk) | **high** | expected **−15 to −20 % B2b** | unchanged; the remaining structural win on B2b. Multi-level TCO falls out for free (`doc/tail_recursion.md`). |
| 9 | optional cleanup | low | 0 | continuation ops; cosmetic |
| 11 | Name::values_ pvalue cache | medium | **measured: B5 16.34 → 1.51 s (−91 %), B7 −4 %, B3 −13 %, B10 −1 %** | ✓ done (commit `aa14150` for the !NDEBUG gate; the cache itself landed earlier in Slice 11 commits). Single biggest perf win to date. |

Stale rows from the old dashboard:
| 2 | ~20 nested-call sites | low | 0 | audit |
| 3 | dispatcher locals | medium | -0.05–0.1 s B2b | aliasing hazards |
| 4 | dispatcher 4-5 sites | medium | -0.05 s B2b | sentinel shim |
| 5 | sli_stack.cpp (~10 ops) | low | -0.08 s B1 | first ABI conversion |
| 6 | 7 files (~300 ops) | moderate | -0.15–0.25 s B2b | bulk conversion |
| 7 | dispatcher cleanup | low | 0 | remove dual-ABI |
| 8 | dispatcher rewrite | **high** | -0.10–0.15 s B2b | inline body walk |
| 9 | optional cleanup | low | 0 | continuation ops |

**Total predicted B2b:** 1.91 s → ~1.55 s. Total predicted
B1: 1.33 s → ~1.25 s.

---

## Things that can break and what to test

1. **Error attribution.** `$errordict /commandname` should be
   the same name as today, except for cases documented in
   slice 2. Test: `test_errors_dispatch.cpp` enumerates ~30
   error scenarios; all must produce the same /commandname
   after every slice.
2. **`$errordict /estack` snapshot shape.** After slice 8,
   the e-stack does not contain individual operator Tokens.
   The snapshot shape changes. Audit:
   - Vendored `lib/sli/*.sli` for any read of `/estack`. Grep:
     `grep -rn "errordict\s*.*estack" lib/sli/`. Expected
     hits: 0 — error printing uses `/command` and
     `/errorname`, not `/estack`.
   - Tests: `grep -rn "/estack\|errordict.*estack" tests/`.
   - If anything reads `/estack`, decide: (a) synthesize the
     missing operator entries when building the snapshot in
     `raiseerror(Name,Name)`, or (b) accept the shape change
     and update tests.
3. **`stop` / `stopped` unwinding.** `stop` walks the e-stack
   looking for a `stopped` mark and pops everything above
   it. The marks are still there; the operator Tokens
   between them are gone (slice 7+). Test: `test_errors.cpp`
   exercises stop/stopped at various depths.
4. **`backtrace`.** `SLIFunction::backtrace` overrides
   (`sli_builtins.cpp:79, 153, 205, 265, 322, 378`) read the
   e-stack expecting specific layouts. After slice 8 these
   layouts change. Update each override.
5. **Recordstacks cost.** B6-family benches measure the cost
   of `raiseerror` with `recordstacks=true`. With the new
   ABI the e-stack is smaller; snapshot cost should drop.
   Track B6/B6b/B6c — they should improve or stay flat.
6. **`cycles`** — counts dispatch iterations. The new
   topology may count differently (one cycle per ref in the
   body walk, not one per outer-switch round-trip). If
   anything user-visible depends on the count, document the
   change.
7. **`SLI3_STATS=1` output.** Operator call counts are
   incremented in the dispatcher (`++call_counts_[t.data_.
   func_val]`). After slice 7, the increment moves with the
   call site. Ensure it still fires once per call.
8. **Memory: e-stack peak depth.** Without per-op pushes the
   e-stack is shallower. Some tests assert depth ≤ N for a
   given workload (`test_dispatch_parity.cpp` may); update
   asserts.

---

## What lives in each commit (PR-level structure)

If we land via PRs, one PR per slice. PR descriptions:

- **PR-1 (slice 0):** "Baseline measurements for Axis I work"
  — adds a notes file, no code changes.
- **PR-2 (slice 1):** "Add current_op_ field; reroute
  raiseerror via it" — pure refactor.
- **PR-3 (slice 2):** "Audit nested-op attribution sites" —
  documents call sites; small code edits at most.
- **PR-4 (slice 3):** "Lift dispatcher locals into registers"
  — performance refactor with bench data.
- **PR-5 (slice 4):** "Replace functiontype e-stack push with
  sentinel" — staging step.
- **PR-6 (slice 5):** "Convert sli_stack.cpp to new operator
  ABI" — first ABI batch.
- **PR-7 (slice 6, ×7 sub-PRs or one PR):** "Convert
  remaining builtins to new operator ABI" — bulk
  conversion.
- **PR-8 (slice 7):** "Remove dual-ABI shim; new ABI is
  required" — cleanup.
- **PR-9 (slice 8):** "Inline procedure-body walk in
  dispatcher" — the structural change.
- **PR-10 (slice 9, optional):** "Convert iter ops to
  continuation operators" — cleanup.

Each PR has its own bench-data section in the description.

---

## Stop conditions

Pause and re-investigate at any of:

- Bench regression at any slice (B1 / B2 / B2b / B3 / B5
  worse than at slice 0).
- `ctest` failure that isn't trivially due to the slice's
  intended behavior change.
- A correctness divergence between old and new dispatchers
  (compared in slice 8's parity test).
- Memory growth: peak RSS during `test_round_trip` or
  bench/run.sh increases by > 10 %.

If any of these triggers, post the data in the PR comment
and stop further slices until resolved.

---

## What this plan does *not* do

- Axis II (hot-op body inlining). Separate plan; depends on
  slice 8 being in place to be cleanly implementable.
- Axis III (compact procedure storage). Separate plan;
  benefits most after Axis I + II are done.
- Computed-goto threaded dispatch. Out of scope; revisit
  after Axes I + II if the perf gap to gs isn't closed.
- Tail-call optimisation for SLI-defined procedures (`prst:`/
  `pr:`/`out:` in gs). The structure in slice 8 lays the
  groundwork; a follow-up could specialise the "proc ends,
  parent is also a proc" case to skip the outer-loop
  re-entry. Marginal extra win; defer.

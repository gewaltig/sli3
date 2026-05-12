# sli3 — consolidated next-step roadmap

Revision date: 2026-05-12 (post-Slice-11). Anchor: commit
`aa14150` (Slice 11 Name(long) bounds-check gate landed).

> **2026-05-12 update**: the original Axis I/II/III ordering has
> been **re-prioritised** after B7-B10 organic-workload data and
> profiling showed the original cost model was stale. See
> `doc/dispatch_restructure_plan.md` "Re-costing (2026-05-12)"
> for the analysis. Key changes from the 2026-05-11 draft:
>
> - **Slice 11 (Name(long) bounds-check gate) shipped**
>   (commit `aa14150`). Net effect: clean −8 to −12 % on B1,
>   B2, B3, B5, B7, B8, B10 (every bench that does any name
>   dispatch); B2b and B9 see smaller wins. After Slice 3
>   + 11, sli3 wins against gs on 5 of 9 benches; B1/B2/B3/B5
>   are now significant wins (−21 to −40 %).
> - **Axes I slices 4-7 collapse into one "Axis I bundle"**
>   that ships as a unit. The per-slice bench gates are
>   unmet in isolation (each slice pays a setup cost; only
>   the combined net is favorable).
> - **Bench-measurement hygiene**: any `echo ... | ./build/sli3`
>   pipeline without a trailing `quit` in the input hangs in
>   REPL at 100 % CPU. The first ~12 hours of Slice 3 /
>   Slice 11 measurements were contaminated by two such
>   orphans (one from each of two early sanity-check `Bash`
>   commands that used `run_in_background` and never returned).
>   Clean numbers below replace the orphan-contaminated ones
>   in the Slice 3 + Slice 11 commit messages.

## Clean bench standing (post-Slice-11, no orphan contamination)

Best-of-five wall-time (`/usr/bin/time -p`, real), Apple Silicon,
AC power, no background load:

| Bench | sli3 | gs 10.07 | sli3 vs gs |
|-------|-----:|---------:|-----------:|
| B1  1 pop                     | 1.03 | 1.31 | **−21 %** (sli3 wins) |
| B2  1 1 add pop               | 1.99 | 2.69 | **−26 %** (sli3 wins) |
| B2b bound proc                | 1.79 | 1.22 | +47 % (sli3 trails) |
| B3  nested for                | 1.74 | 2.57 | **−32 %** (sli3 wins) |
| B5  dict alloc + lookup       | 1.50 | 2.49 | **−40 %** (sli3 wins) |
| B7  bubble sort               | 2.19 | 1.99 | +10 % (sli3 trails) |
| B8  insertion sort            | 1.53 | 1.01 | +51 % (sli3 trails) |
| B9  recursive fib(28)         | 2.19 | 1.71 | +28 % (sli3 trails) |
| B10 matmul 50x50              | 1.82 | 1.89 | **−4 %** (sli3 wins, narrowly) |

5 wins, 4 trails. The remaining gaps (B2b, B7, B8, B9, B7 +10 %)
trace to the same dispatcher overhead: per-operator push + self-pop
+ name handle traffic. The Axis I bundle (drop fn-Token push +
drop 316-site self-pop) is the next target.

Revision date (original): 2026-05-11. Anchor: tag `stage9-complete`.

This document is the single entry point for the next phase of sli3
work. It supersedes the "What to compact next" sections of
`doc/hot_ops.md` and the "Next-step infrastructure" paragraphs in
`implementation_spec.md`. It does **not** replace
`doc/compact_procedure_spec.md` (Axis I/II/III specification) or
`doc/dispatch_restructure_plan.md` (sliced Axis I plan) — those remain
the implementation-level references. This document sequences them and
adds proposals derived from `doc/gs_reference.md` that are not yet
captured anywhere else.

## Current state (one screen)

- Correctness work through `fix-plan.md` Stages 0-5, 7, 9 done. Stages
  6 (serialization gaps) and 8 (cleanup) partially open — see
  `code-review.md#resolution-status`.
- Stage 9 trie compaction done in 10 batches; 33 ops compacted; B2
  per-iter cost went from 23.8 ns to 17.7 ns.
- `savestate` / `restorestate` operators landed (commit `3a7edec`);
  dict-stack snapshot blocked on `DictionaryType::serialize` (Stage 6).
- Bench standing vs gs 10.07: tied or ahead on B1/B2/B3/B5; **trailing
  on B2b** (bound-procedure repeat) by +55%. Closing B2b is the
  driving goal of this phase.
- Axis I dispatcher restructure (`doc/dispatch_restructure_plan.md`)
  is specified, eleven slices, not started. The latest commit added
  the planning docs and the savestate/restorestate operators as a
  forward-compatible precursor.

## Goal of this phase

Close the B2b gap to gs (target: B2b ≤ 1.30 s, within 10% of gs's
1.23 s) and keep the wins on B1/B2/B3/B5. The path runs from
**structural dispatcher changes** to **inlining bodies** to
**packed storage**, in that order. Each step depends on the previous
one being in place.

## Roadmap — committed track

These are the plans already specified. Sequence them, do them in
order, and 80% of the gap closes without new design work.

### Track A1 — Axis I (dispatcher restructure)

Reference: `doc/compact_procedure_spec.md` §Axis I,
`doc/dispatch_restructure_plan.md` (11 slices), `doc/tail_recursion.md`
for the multi-level TCO design recommendation.

Slice mnemonics (read the plan for full detail):

0. Baseline measurements.
1. Add `current_op_` field; re-route raiseerror via it.
2. Audit nested-op attribution sites.
3. Lift dispatcher locals into registers (`iref`, `icount`, `iosp`,
   `iesp`).
4. Replace functiontype e-stack push with sentinel (staging step).
5. Convert `sli_stack.cpp` to new operator ABI (first batch).
6. Convert remaining `src/builtins/*.cpp` to new ABI (~7 files,
   ~300 sites).
7. Remove the dual-ABI shim.
8. Inline procedure-body walk into the dispatcher (gs's `top:`
   topology with `bot:` / `out:` / `up:`).
9. (Optional) convert iter operators to continuation operators.
10. `savestate` / `restorestate` — already landed in `3a7edec`.

Bench delta target: B2b 1.91 s → ~1.55 s. B1 1.33 s → ~1.25 s.

**Key state invariants** (from Slice 0 of the plan): stacks are
authoritative at logical boundaries; register state must sync before
op calls / throws / exits; `current_op_` is transient; e-stack shape
changes from "operator Tokens interspersed with iter frames" to "iter
frames + continuation ops only". Auditing those four invariants is
what catches subtle bugs introduced by the restructure.

### Track A2 — Axis II (hot-op inlining)

Reference: `doc/compact_procedure_spec.md` §Axis II.

After A1, the dispatcher's main switch is the right place to inline
the bodies of `pop`, `dup`, `exch`, `add` (integer arm), `sub`, `index`,
`roll`, `def`, `if`, `ifelse`, `for`, `repeat`. `HotOpId` enum on
`SLIFunction`; switch arms read `fn->hot_op()` before falling back to
`fn->execute()`.

Bench delta target on top of A1: B2b → ~1.35 s.

### Track A3 — Axis III (compact procedure storage)

Reference: `doc/compact_procedure_spec.md` §Axis III.

`CompactProc` (4-byte slots + side pools for doubles / spilled
Tokens), built by a C++ `BindFunction` that replaces the SLI-defined
`bind`. Mutation: decompact-on-write. Serialize: emit as unpacked
TokenArray.

Bench delta target on top of A1 + A2: B2b → ~1.30 s.

## Roadmap — gs-derived proposals (new)

These are not in `compact_procedure_spec.md`. They come from reading
`doc/gs_reference.md` against the current sli3. Each is independent
of A3 and most are independent of A2 — they target different
sources of overhead.

Bench expectations are predictions, not measurements. Land each
behind a measurement gate (`bench/run.sh` before + after).

### Proposal P1 — `pvalue` cache on Name

**What gs does** (`doc/gs_reference.md` §5.4): every `name` struct
has a `pvalue` field that points directly at the dict slot when the
name is bound in exactly one dictionary (the common case for
`systemdict` / `userdict` entries). Lookup is **one pointer
indirection** — no hash walk, no dict-stack walk.

When the cache is invalid (`pv_no_defn` or `pv_other`), gs falls
back to `dict_find_name_by_index_inline` which does the full
dict-stack walk and updates the cache.

**What sli3 does today**: `DictionaryStack::lookup(Name)` walks the
dict-stack list, calling `Dictionary::lookup` on each. The
`DICTSTACK_CACHE` macro caches `Token*` keyed by Name handle to
short-circuit subsequent lookups — but only for names already seen
on the current stack. Cold lookup is still a list walk.

**Why this matters for sli3**: B5 is the only bench that hits the
dict-stack lookup path hot (16.3 s; gs is 24.7 s — sli3 already
wins). B2 and B3 hit `nametype` dispatch *inside the iter case*
(`sli_interpreter.cpp:870-880` and similar). Stage 9 cured the
typical case by binding to `functiontype` via the dispatcher inline
path; unbound `nametype` still walks the dict stack on every
reference.

A `pvalue` cache benefits:
- Unbound procedures (no `bind` step): every name dispatch becomes
  one pointer load instead of a list walk. Big win on REPL workloads.
- Bound procedures where `bind` left some names as `nametype` (per
  PostScript spec — names whose values are procedures or non-operators
  stay late-bound).
- B5-style code where `a` / `b` are user dict names.

**Proposed design**:

```cpp
struct NameValue {
  Token* slot = nullptr;     // 0 = no defn, 1 = multiple defs, else ptr
};

class Name {
  static std::vector<NameValue> values_;  // parallel to table_
  ...
};
```

`DictionaryStack::insert(n, t)` updates `Name::values_[n.toIndex()]`
to point at the slot. `Dictionary::erase(n)` clears it. If a name is
defined in two dicts on the stack, the second `insert` flips it to a
"multi" sentinel; the dispatcher falls back to the existing
list-walk path.

**Estimated bench delta**: B2 unbound case is dominated by `add`
dispatch (now compact), so the win is small (~2-3 ns/iter on B2 if
the `1` literal pushes don't beat the name-dispatch path). B2b
benefits via the `pop` and `add` lookups inside the bound body when
`bind` decided to leave them as names (it shouldn't, but the
slot encoding may include some). B5 should see a noticeable drop —
estimate 16.3 s → ~12-13 s.

**Risks**:
- `Name::values_` table grows with every interned name; static array
  vs `std::vector` tradeoff. Cap by `Name::table_.size()`.
- Cache coherence across `clear`/`undef`/`begin`/`end`. Every dict
  operation that adds or removes a binding touches the cache. There
  are ~10 such call sites; audit before shipping.
- Interaction with the existing `DICTSTACK_CACHE`: pick one or
  layer them. Simplest: keep `DICTSTACK_CACHE` for stack-aware
  scoping; `pvalue` is the fast path for the unambiguous case.

**Ordering**: independent of A1/A2/A3. Could land before A1 as a
standalone perf improvement, especially if B5 becomes a target
workload. Recommended placement: **after A1 slice 7** (operator ABI
clean, before the dispatcher restructure in slice 8). The
`pvalue` cache makes nametype dispatch cheaper, which slice 8's
inline-body-walk also wants to be cheap.

### Proposal P2 — operator return-code protocol

**What gs does** (`doc/gs_reference.md` §5.2): operators in gs do
not push themselves onto the e-stack or pop themselves; they return
an integer code that tells the dispatcher what to do next:

| code | meaning |
|------|---------|
| 0, 1 | normal success; advance to next ref |
| `o_push_estack` | I pushed onto e-stack; please continue from new top |
| `o_pop_estack` | done with my e-stack frame; pop it |
| `gs_error_Remap_Color` | special path |
| < 0 | error code; raise |

The operator function pointer signature is
`int (*op_proc_t)(i_ctx_t* i_ctx_p)`. The dispatcher's
`call_operator` site reads the code and branches into the right
follow-up path. Operators never touch the e-stack pointer for
self-pop / self-push — only for *real* state pushed during their
work.

**What sli3 does today**: every `SLIFunction::execute` ends with
`i->EStack().pop()` (the self-pop convention, ~316 sites).
Operators that push iter frames or continuation state push them
explicitly. Errors are raised via `raiseerror` which throws.

**Why this matters**: A1 Slice 5/6 already converts the ABI to "no
self-pop" (the dispatcher pops the operator's Token after the call
returns). That's most of the way to gs's protocol. The remaining
gap is that some operators need to communicate "I pushed state, the
dispatcher should keep going" vs "I'm done, please pop my mark"
without the e-stack shape doing the communicating.

**Proposed design**: extend `SLIFunction::execute` from `void` to
`int`:

```cpp
enum class OpResult : int {
  Success,        // 0 — normal; advance
  PushEstack,     // op pushed frames; dispatcher should re-enter via up:
  PopEstack,      // op consumed its frame; dispatcher should advance parent
};

class SLIFunction {
  virtual OpResult execute(SLIInterpreter*) const = 0;
};
```

The dispatcher reads the return value:

```cpp
OpResult r = fn->execute(this);
switch (r) {
  case OpResult::Success:     /* advance iref */ break;
  case OpResult::PushEstack:  /* re-examine e-stack top */ break;
  case OpResult::PopEstack:   /* pop parent frame */ break;
}
```

Most operators return `OpResult::Success`. `repeat` / `for` / `loop`
return `PushEstack` to tell the dispatcher to enter the body they
just set up. Errors continue to throw.

**Estimated bench delta**: ~0-1 ns/iter. The win is correctness-of-
architecture, not raw cycles. The new return-code path eliminates a
class of latent bugs where an operator pushes a frame and forgets to
signal "now dispatch it" (currently signaled by the e-stack top tag
happening to be an iter marker).

**Risks**:
- 316 `execute` signatures change. Most return `Success`; few need
  more thought. Mechanical edit, low risk per site.
- Throwing-from-execute path becomes throwing-or-returning. Some
  operators today both throw and pop (post-Stage 7); after this
  change, throw still pops the e-stack via the catch handler.
  Clean up.

**Ordering**: Naturally lands during A1 slice 6, when every operator
is touched anyway. Cheap addition to a mechanical edit; don't
amortize it into a separate pass.

### Proposal P3 — continuation-op iter style

**What gs does** (`doc/gs_reference.md` §6): `repeat` / `for` /
`loop` / `forall` set up an e-stack frame `[mark, count, proc,
continue_op]` and return `o_push_estack`. The continuation op
(`repeat_continue`, `for_pos_int_continue`, …) is a regular C
operator that sits on top of the e-stack between iterations. After
each body finishes, the dispatcher's `up:` arm dispatches the
continuation op, which either pushes another copy of the proc and
returns `o_push_estack` or pops the frame and returns
`o_pop_estack`.

There are **no `iiteratetype` / `irepeattype` / etc. type markers**.
Iter state is regular C operators + state Tokens on the e-stack.

**What sli3 does today**: each iter type
(`iiteratetype` / `irepeattype` / `ifortype` / `iforalltype`) is an
enum slot in `sli_typeid`. The dispatcher has a switch case per
iter type. The case-body iterates the procedure inline, picking
`proc` and `pos` from the e-stack frame.

**Why this matters**: in A1 slice 8, the dispatcher becomes the
body-walk loop. The iter cases collapse — there's no longer a
distinct "I'm in repeat mode" code path; there's one "walk this
body, advance, repeat" loop. But the e-stack still has to carry
state. With type markers, the dispatcher walks them via the outer
switch's "what's on the e-stack top" path; with continuation ops,
the dispatcher dispatches them via the same fast path as any other
operator.

Converting to continuation ops:
- Removes 4 enum slots and 4 switch cases.
- Makes loops uniform with other operators.
- Removes the need for the "is the e-stack top still my iter marker?"
  check in slice 8's `bot:` / `up:` arms (today the check is
  implicit via the case structure; after the conversion, `up:`
  always dispatches whatever's on top, including continuation ops).

**Proposed design**: per `dispatch_restructure_plan.md` §Slice 9
(optional). The continuation-op classes (`RepeatContinue`,
`ForContinue`, …) are SLIFunctions like any other. The iter-type
markers become trivial sentinels or get removed.

**Estimated bench delta**: 0 perf, simplifies the dispatcher.
Worth doing if A1 slice 8 lands cleanly; defer otherwise.

**Risks**:
- Wire-contract change: `sli_typeid` slot renumbering breaks every
  saved snapshot. Either pin the slots and just don't use them, or
  bump `kSerializeVersion`.
- Backtrace overrides (`SLIFunction::backtrace`) currently walk
  iter-type frames. They'd walk continuation-op frames instead.
  Minor rewrite per the four iter types.

**Ordering**: after A1 slice 8 has landed cleanly. Optional polish.

### Proposal P4 — multi-level TCO cascade

**What gs does** (`doc/tail_recursion.md` §2, `doc/gs_reference.md`
§3.4): gs's TCO is structural, not a special case. `icount` is
initialized to `r_size - 1` on body entry, so the last ref of every
body is dispatched after the body frame is popped via `out:`. If
the last ref enters a new procedure, the new frame replaces the
just-popped one — zero net depth change. If the parent body also has
just one ref left, `up:` cascades: pop, dispatch, check again.

Multi-level TCO collapses a chain of N procedures-each-calling-the-
next in O(1) dispatcher iterations, not O(N).

**What sli3 does today** (`doc/tail_recursion.md` §1): TCO is
implemented inside the `iiteratetype` case only. After fetching the
last ref of the current body, the dispatcher overwrites the proc
slot in place, pops the iter marker, and `break`s — the outer
switch re-dispatches on the new top.

Limitations:
- Only `iiteratetype` — not `irepeattype` / `ifortype` /
  `iforalltype` (their frames are reused across iterations, so the
  "tail" semantic doesn't apply per-iteration; but the body's tail
  token in any of these doesn't get TCO either).
- One frame per tail call. Deep chains of "outer ends with inner;
  inner ends with innermost; ..." take N dispatcher iterations to
  collapse.

**Why this matters**: rare on the bench surface (none of B1/B2/B3/
B5 exercise deep TCO). Matters for SLI scripts with many small
helper procs that mutually call each other in tail position.
Stylistically, gs's design is cleaner — it falls out of the
`bot:` / `out:` / `up:` structure rather than being a special arm.

**Proposed design**: A1 slice 8's recommended approach (per
`doc/tail_recursion.md` §Implications) is to adopt gs's
`bot:` / `out:` / `up:` topology. If we do that, multi-level TCO
falls out for free. Specifically:
- Initialize `icount = body_size - 1` on body entry.
- `next()` decrements `icount`; falls to `out:` when icount goes
  from 1 to 0 (the second-to-last ref was just dispatched).
- `out:` pops the body frame and advances `iref` to the last ref.
- The last ref is dispatched with the body frame already popped —
  whatever it is. Procedure entry naturally tail-recurses.
- `up:` cascades: if the parent body had 0 or 1 refs left, pop it
  too; repeat.

**Estimated bench delta**: 0 on the current B-series (no deep TCO
exercised). On a tail-recursive benchmark (e.g. `loop_fn { ...
loop_fn } def`) the win is from O(N) → O(1) on deeply nested
chains.

**Risks**: structural change to the dispatcher; rides with A1
slice 8. Already in scope of that slice per `doc/tail_recursion.md`
Implications §recommendation **(b)**. Track this as "TCO must remain
correct or improve through slice 8" — not as a separate slice.

**Ordering**: rides with A1 slice 8. Not a separate workstream.

## Sequencing summary (revised 2026-05-12)

```
Done:
  Axis I Slice 0 — baseline                                    (note in /tmp/axis1-baseline.md)
  Axis I Slice 2 — nested-op attribution audit                 (doc/axis1_slice2_audit.md)
  Axis I Slice 3 — pos pointer hoist in iter cases             (commit a9c3d25)
  Bench expansion — B7-B10 organic workloads                   (commit 31d6518)

Next (re-prioritised based on profile data):

  1. Slice 11 (P1 pvalue cache).
     Standalone, biggest win on B5/B7/B10 (~−15-20 %) plus
     incidental win on B9. Independent of Axis I bundle.

  2. Axis I bundle (was Slices 4-7).
     Single bundle: add current_op_, drop fn-Token push, drop
     316-site self-pop, remove dual-ABI shim. Per-step ctest
     gate; bench gate only at end of bundle.

  3. Axis I Slice 8 (inline body walk).
     Biggest structural win on B2b (~−15-20 % expected).

  4. Axis I Slice 9 (continuation ops).
     Optional cosmetic cleanup.

  5. Axis II (hot-op inlining, per doc/compact_procedure_spec.md).
     ~3-5 % B9 from inlining lt/dup/sub/add/exch/ifelse.

  6. Axis III (compact procedure storage, per doc/compact_procedure_spec.md).
     Last and smallest. Only if B2b after Axes I + II still > 10 %
     above gs.

Correctness backlog (parallel track):
  Stage 6 — close serialization gaps (DictionaryType, TrieType,
            ProcedureType, OperatorType, null-sentinel
            disambiguation, stream object-table)
  Stage 8 — close cleanup items (::pop binding, Cvt_a, Parser
            dtor, DFA static, signal handler, argv,
            intvector*type)
```

The Phase 2/3 ordering boundary is soft. P1 (pvalue cache) is
independent of A1 and could be Phase 2's first commit if B5 becomes
a hot bench target. The remaining proposals are best done in the
order shown because each depends on the dispatcher having the gs-
style topology that A1 introduces.

## Acceptance gates

Per-phase gates (in addition to per-slice gates already in
`dispatch_restructure_plan.md`):

- **Phase 1**: every `sli_typeid` round-trips (close every `SKIP` in
  `test_round_trip.cpp`); `test_state_ops.cpp` covers dict-stack
  round-trip; CTest fully green; `code-review.md#open--stage-6` and
  `code-review.md#open--stage-8` empty.
- **Phase 2 A1**: B2b ≤ 1.55 s; B1 ≤ 1.25 s; CTest green; no
  regression on B5.
- **Phase 2 A2**: B2b ≤ 1.35 s; CTest green.
- **Phase 2 A3**: B2b ≤ 1.30 s; CTest green; compact storage
  documented for users of `cva` / `length_p` / `==` printing.
- **Phase 3 P1**: B5 ≤ 13 s; no regression elsewhere; CTest green.

## What this plan does not cover

- Reintroducing `sli_processes` (deferred per CLAUDE.md Q4).
- The `iteratortype` / `forall_iter` protocol (out of scope per
  CLAUDE.md).
- Multi-threading. Single-threaded is the design choice.
- Computed-goto threaded dispatch. Earlier experiments showed no
  win on M2; revisit only if Axis I + II + III leave a measurable
  gap to gs in the dispatch loop body.
- JIT / bytecode compilation. The current dispatcher is fast
  enough that JIT is unmotivated until and unless Axis I/II/III
  fail to close the gap.

## File map for this phase

| What | Path |
|------|------|
| Roadmap (this document) | `doc/next_steps.md` |
| Axis I sliced plan | `doc/dispatch_restructure_plan.md` |
| Axis I/II/III spec | `doc/compact_procedure_spec.md` |
| gs reference notes | `doc/gs_reference.md` |
| TCO comparison | `doc/tail_recursion.md` |
| Operator catalog & bench standing | `doc/hot_ops.md` |
| Correctness status | `code-review.md` (resolution status at top) |
| Per-stage TDD plan | `fix-plan.md` |
| Master log / Decisions | `implementation_spec.md` |
| Bench harness | `bench/run.sh`, `bench/sli/`, `bench/ps/` |

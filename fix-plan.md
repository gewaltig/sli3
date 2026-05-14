# sli3 fix plan

Forward-looking work list. Pre-Axis-I/II stages are closed; the
stage-by-stage TDD detail lived useful service through `stage9-complete`
and is now retired (see git history). This document tracks **what's
still pending** plus the new findings from the 2026-05-13 review.

For correctness items see `code-review.md`; for performance roadmap
see `doc/next_steps.md`, `doc/compact_procedure_spec.md`, and
`doc/dispatch_restructure_plan.md`.

---

## Status snapshot (2026-05-13)

| Track | Status |
|---|---|
| Stages 0–5 (test infra → type/container/error → bootstrap → math/scanner) | ✅ closed |
| Stage 6 (serialization) | ▶ partial — `FunctionType` done; `DictionaryType` / `TrieType` / `ProcedureType` / `LitprocedureType` / stream-aliasing / null-sentinel still open |
| Stage 7 (dispatcher parity & static-cache lifetime) | ✅ closed — `static SLIType*` and the trie `static SLIInterpreter*` cache both retired |
| Stage 8 (cleanup / hardening) | ▶ partial — see "Pending cleanup" below |
| Stage 9 (compact hot ops) | ✅ closed (tag `stage9-complete`) |
| Stage 10 (savestate/restorestate) | ✅ closed (commit `3a7edec`) — dict-stack snapshot blocked on Stage 6 closure |
| Stage 11 / Axis I bundle (dispatcher pre-pop ABI) | ✅ closed (commit `151e5e5`) |
| Stage 12 / Axis I slice 8 (unified body-walk loop) | ✅ closed — steps 1+2 (commits `470da6d` + `8e39906`); step 4 (flip default + delete OFF path) landed 2026-05-14; step 3 (optional inline-nested-proc) deferred |
| Stage 13 / Axis II (hot-op inlining) | ▶ step 2 + step 3 done — 16 ops tagged (pop/dup/exch/add_ii/add/sub/if/def/gt/lt/geq/leq/eq/neq/get/put). Symmetric in both functiontype and name-resolved hot-op switches |
| Axis III (compact procedure storage) | ⏳ not started; depends on Axis I+II being stable |
| 2026-05-13 follow-up pass | ✅ closed — CRITICAL `Cvt_aFunction` + 9 HIGH items (restorestate atomic, cleardict cache, where, typed-leaf type checks, execute(string), execute_debug_, startup(), terminate(), nulltype guard) |

Build clean on AppleClang 21 / C++17 (both OFF and ON modes);
**20 / 20 tests passing**; zero CRITICAL regressions in this
review pass.

---

## Bench standing (best-of-five, sli3 vs gs 10.07 vs nest 2.20)

Latest run: `2026-05-14`, commit `90b129c` (post-Axis-II step 3),
host `theo`. Stored in `bench/results.db`; reproduce with
`bench/summary.sh --md`. Δ vs the post-step-2 baseline
(`80d5695`) is shown in the last column.

| Bench | sli3 | gs | nest | sli3 vs gs | Δ baseline |
|---|---:|---:|---:|---:|---:|
| B1   `1 pop`           | **0.86** | 1.33 | 1.98 | **−35 %** ⬇ | +1.2 % |
| B2   `1 1 add pop`     | **1.83** | 2.74 | 4.38 | **−33 %** ⬇ | +1.1 % |
| B2b  bound `{...}`     | 1.73 | **1.25** | 3.38 | +38 % ⬆ | **+12.3 %** ⚠ |
| B3   nested for        | **1.53** | 2.56 | 4.26 | **−40 %** ⬇ | +0.0 % |
| B5   dict alloc+lookup | **1.47** | 2.50 | 4.34 | **−41 %** ⬇ | −1.3 % |
| B7   bubble sort       | 2.18 | **1.99** | — | +10 % ⬆ | +1.4 % |
| B8   insertion sort    | 1.41 | **1.01** | — | +40 % ⬆ | **−4.1 %** ⬇ |
| B9   recursive fib(28) | 2.03 | **1.71** | 4.26 | +19 % ⬆ | **−3.8 %** ⬇ |
| B10  matmul 50×50      | **1.74** | 1.90 | — | **−8 %** ⬇ | **−2.8 %** ⬇ |

Score vs gs: **5 wins, 4 losses** — unchanged from baseline.
sli3 beats nest 2.2–2.9× across the board. Axis II step 3
delivered the predicted wins on the organic workloads (B8 / B9 /
B10, the ones that actually use gt / lt / get / put inside their
inner loops). B7 did not improve as much as predicted; B2b
regressed +12 % even though it uses none of the newly-tagged ops
— see "B2b regression follow-up" below.

---

## Pending correctness work

Severity tags match `code-review.md`. Each item is a one-line fix
hint; the diagnostic detail lives in `code-review.md`.

### CRITICAL — closed in 2026-05-13 follow-up pass

All historical CRITICAL items are now closed; `Cvt_aFunction`
walks the array via `TypeNode::from_token_array` (commit pending).

### HIGH — dispatcher / runtime — closed in 2026-05-13 follow-up pass

All eight HIGH dispatcher/runtime items from the prior pass are
now closed:
- `RestoreStateFunction` buffers into local `TokenStack`s before
  swap (`sli_state_ops.cpp`).
- `CleardictFunction` invalidates dictstack cache / basecache
  before erasing on-stack dicts (`sli_container_ops.cpp`).
- `WhereFunction` walks the dictstack top-down via new
  `DictionaryStack::where(Name)` returning `Dictionary*`.
- `Inv_dFunction` and the rest of the typed math leaves
  (`sin_d`/`cos_d`/`asin_d`/`acos_d`/`exp_d`/`log_d`/`ln_d`/
  `sqr_d`/`sqrt_d`/`pow_dd`/`pow_di`/`modf_d`/`frexp_d`/
  `ldexp_di`/`dexp_i`/`abs_i`/`abs_d`/`neg_i`/`neg_d`) gained
  `require_stack_type` guards.
- `execute(const std::string&)` now routes through
  `execute_dispatch_()`.
- `execute_debug_` calls `raiseerror(exc)` instead of swallowing;
  exitcode lookup guarded the same way as the main dispatcher.
- `startup()` flips `is_initialized_` unconditionally.
- `terminate()` no longer self-deletes; std::exit alone suffices.

### HIGH — body-walk inline (Axis I slice 8) hardening — closed

`case sli3::nulltype: execution_stack_.pop(); break;` added to
both dispatchers' outer switches as a defensive guard against a
future old-ABI op leaking the sentinel onto the e-stack
(`sli_interpreter.cpp`).

### HIGH — Stage 6 closure (serialization)
10. `DictionaryType::serialize` / `deserialize` —
    `src/types/sli_dicttype.cpp`. Write key count + per-entry
    (Name + Token); deserialize symmetric with object-table dedup
    for the dict itself.
11. `TrieType::serialize` / `deserialize` —
    `src/trie/sli_trietype.h`. Write the trie's name; re-resolve
    via dictstack (mirror `FunctionType::serialize`).
12. Procedure vs litprocedure vs array typeid round-trip —
    `src/types/sli_arraytype.cpp`. Either give each its own
    override or document a shared wire format that records the
    discriminator. Today cross-typeid aliasing silently coerces.
13. Stream-aliasing register —
    `src/types/sli_iostreamtype.cpp:53-66`. Register stream
    Tokens in the Reader's object table on deserialize so aliased
    streams load as a single closed-stream object.
14. `write_token` null sentinel ambiguity —
    `src/serialize/sli_serialize.cpp:152-162`. Reserve a separate
    sentinel byte or shift `nulltype` out of typeid slot 0.
    Wire-format break — do it before any saved snapshots exist.
15. Test xfails — flip Dictionary / Trie / Procedure /
    OperatorType markers in `tests/test_round_trip.cpp` to
    expect-pass as each Stage 6 gap closes. Add a cycle test,
    a version-mismatch test, and a multi-token-stream test to
    `tests/test_serialize.cpp`.

---

## Pending cleanup

Items the previous Stage 8 list flagged that are still open. None
are blocking, but they make the next round of changes easier.

### Dispatcher / hot path
- `SLI3_TRACE` block duplication ✅ resolved by slice 8 step 4
  (only one dispatcher remains).
- `sli_op_bodies.h` — change `static inline` to `inline` to allow
  ODR merging across the 4 TUs that include it.
- Slice 8 step 4 ✅ landed 2026-05-14 — flipped to ON-only, OFF
  body deleted (~575 lines of OFF-mode dispatcher gone), flag
  removed from CMakeLists.txt, `execute_dispatch_inline_`
  renamed back to `execute_dispatch_`.

### Container / parser
- DFA `trans` is per-instance; move to `static const` —
  `src/parser/sli_scanner.h:120`.
- `class CharCode : public std::vector<size_t>` — switch to
  composition — `src/parser/sli_charcode.h:33`.

### Builtins / cleanup
- `CloseStreamFunction` old-ABI — flip to new-ABI for uniformity
  — `sli_io_ops.cpp:121-142`.
- `getinterval_a/s` empty-trailing-interval rejection —
  `sli_container_ops.cpp:747-811`. Match NEST 2.20.2.
- `MapFunction` / `MapIndexedFunction` empty-proc behaviour —
  `sli_array_module.cpp:710-731`. Verify parity vs NEST 2.x
  (which returns a copy).
- `CvaFunction` trie arm leaves extra slot —
  `sli_container_ops.cpp:1426`.
- `locate_sli_init` second probe —
  `sli_startup.cpp:390-402`.

### Dead code
- Eight `backtrace` overrides in `sli_builtins.cpp` (and matching
  declarations in `sli_builtins.h:58-115`).
- `step_mode()` always false + its dangling-reference call sites
  in `sli_builtins.cpp:406-413, 451-456`.
- `extern int signalflag` (read at 4 dispatcher sites, written
  nowhere) — either implement signal handling in `sli_main.cpp`
  or remove the dead reads.

### Misc
- `class SystemSignal` collides with `Name SystemSignal` member
  — rename the exception class.
- `intvectortype` / `doublevectortype` placed after
  `num_sli_types` — either implement (with `types_.resize`
  bumped) or delete. Placeholder OOB risk.
- `sli_main.cpp` — propagate argv into the interpreter; today
  `init_internal_functions` hard-codes `init_slistartup(this, 0,
  nullptr)`.
- `SLIModule` declarations (`sli_module.h:53, 58`,
  `sli_interpreter.h:1064-1071`) have no definitions in `src/`
  — either bring the .cpp back from `unported/` or remove the
  declarations.

---

## Performance follow-on

The Axis I + II axes are partially deployed; remaining wins map to
the three options below.

### Axis II step 3 — tag more hot ops ✅ landed 2026-05-14
8 new ops tagged: `gt`, `lt`, `geq`, `leq`, `eq`, `neq`, `get`,
`put`. Bodies live in `src/builtins/sli_op_bodies.h`; the
standalone `*Function::execute()` methods delegate. Both
dispatcher hot-op switches (functiontype + name-resolved) got
arms. Incidental: fixed two latent refcount leaks in eq/neq and
the string-compare arm of compare (slot-1 type was being
overwritten as booltype without releasing the refcounted
payload).

Bench result vs the seeded baseline: B8 / B9 / B10 closed −3 to
−4 % (predicted −5 to −10 %, on the low side). **B2b regressed
+12 %** even though it uses only `add` / `pop` (both already hot
ops before step 3). The dispatcher hot-op switch growing from 8
to 16 cases is the likely cause — B2b is the most dispatch-dense
bench (tight `{1 1 add pop} bind repeat`, no name lookups) and
absorbs every per-dispatch nanosecond. See follow-up below.

### B2b regression follow-up — investigated 2026-05-14
Root cause **identified** but **not fully resolved**. Both the
`6208141` review-pass +5 % regression and the `90b129c` step-3
+4 % regression are pure **code-layout / I-cache effects** from
growing the dispatcher's hot-op switches.

Reproducible: applying just the 4 new name-resolved-switch arms
from `6208141` to baseline `80d5695` source pushes B2b from 1.55
to 1.66, even though B2b's bound procedure uses the *functiontype*
switch, not the name-resolved one. Removing them at `6208141`
fully recovers baseline (1.53). Removing all 16 step-3 arms at
`90b129c` also fully recovers baseline (1.54) but loses every
B8/B9/B10 win.

Variants measured at `90b129c`:

| Variant | B2b | B7 | B8 | B9 | B10 |
|---|---:|---:|---:|---:|---:|
| baseline (`80d5695`) | 1.55 | 2.15 | 1.47 | 2.11 | 1.79 |
| HEAD (`90b129c`) | 1.74 | 2.20 | 1.41 | 2.03 | 1.75 |
| fix-e (warm out-of-line) | 1.67 | 2.14 | 1.41 | 2.06 | 1.81 |
| fix-f (drop name-res sw) | 1.73 | 2.20 | 1.44 | 2.06 | 1.72 |

Fix-e: keep the 8 pre-step-3 arms inline in the functiontype
switch; push GT/LT/GEQ/LEQ/EQ/NEQ/GET/PUT and the warm fallback
into an out-of-line `dispatch_warm_or_virtual_` helper. Trade-off:
recovers half of the B2b regression and improves B7, but gives
back B10's step-3 improvement (B10 returns to baseline).

**No "best of both worlds" found** — the dispatcher TU's total
size is a hard constraint. The wider the inline switch, the more
B2b suffers from code-layout drift; the narrower it is, the more
warm ops pay an indirect-call cost in B10 / matmul-style loops.

Recommended approaches for a future fix:
1. **PGO build** (`-fprofile-generate` + B-suite run + `-fprofile-use`).
   Lets the compiler lay out the hot path optimally rather than
   relying on switch position.
2. **Function-pointer dispatch table** indexed by `HotOpId`.
   Replaces the switch entirely; eliminates the jump-table size
   issue. Costs one indirect call per op; depends heavily on
   indirect-branch prediction.
3. **Split the dispatcher across TUs** so each hot-op cluster
   lives in its own compilation unit and is reached via an
   indirect call. Same shape as `dispatch_warm_or_virtual_` but
   with a stricter cold/hot split.

Bench gate before declaring done: B2b ≤ 1.60 s **and** B8 ≤ 1.45 s
**and** B10 ≤ 1.80 s (i.e. recover B2b without giving back any of
the B8/B10 wins).

### Axis III — compact procedure storage
Spec at `doc/compact_procedure_spec.md` §Axis III. Predicted B2b
/ B9 gain ~0.5–1 ns/iter on top of Axis I+II. Prerequisite (slice
8 step 4) landed 2026-05-14; Axis III can start whenever.

### Beyond Axis III
`doc/next_steps.md` lists the gs-derived proposals (pvalue cache,
operator return-code protocol, continuation-op iter style,
multi-level TCO) that are still on the board after Axis III. Out
of scope for this plan until Axis III lands.

---

## Out of scope

- Reintroducing `sli_processes` (deferred per CLAUDE.md Q4).
- The `iteratortype` / `forall_iter` protocol (out of scope per
  CLAUDE.md).
- Multi-threading. The single-threaded design is load-bearing;
  every static counter (`TokenArray::allocations`, `Name::insert`,
  the dispatcher's cycle counter, the Meyers singletons in
  `sli_name.h`) becomes a race if revisited.

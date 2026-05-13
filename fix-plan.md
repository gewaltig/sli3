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
| Stage 12 / Axis I slice 8 (unified body-walk loop) | ▶ steps 1+2 done (commits `470da6d` + `8e39906`); step 3 optional; step 4 (flip default + delete OFF path) pending |
| Stage 13 / Axis II step 2 (hot-op inlining) | ▶ 8 ops tagged (pop/dup/exch/add_ii/add/sub/if/def). Symmetric in both functiontype and name-resolved hot-op switches as of 2026-05-13 |
| Axis III (compact procedure storage) | ⏳ not started; depends on Axis I+II being stable |
| 2026-05-13 follow-up pass | ✅ closed — CRITICAL `Cvt_aFunction` + 9 HIGH items (restorestate atomic, cleardict cache, where, typed-leaf type checks, execute(string), execute_debug_, startup(), terminate(), nulltype guard) |

Build clean on AppleClang 21 / C++17 (both OFF and ON modes);
**20 / 20 tests passing**; zero CRITICAL regressions in this
review pass.

---

## Bench standing (best-of-five, sli3 ON vs gs 10.07 vs nest 2.20)

| Bench | sli3 | gs | nest | sli3 vs gs |
|---|---:|---:|---:|---:|
| B1   `1 pop`           | **0.85** | 1.32 | 2.01 | **−36 %** ⬇ |
| B2   `1 1 add pop`     | **1.81** | 2.69 | 4.34 | **−33 %** ⬇ |
| B2b  bound `{...}`     | 1.54 | **1.22** | 3.39 | +26 % ⬆ |
| B3   nested for        | **1.53** | 2.58 | 4.30 | **−41 %** ⬇ |
| B5   dict alloc+lookup | **1.49** | 2.49 | 4.32 | **−40 %** ⬇ |
| B7   bubble sort       | 2.15 | **1.99** | — | +8 % ⬆ |
| B8   insertion sort    | 1.47 | **1.01** | — | +46 % ⬆ |
| B9   recursive fib(28) | 2.11 | **1.71** | 4.27 | +23 % ⬆ |
| B10  matmul 50×50      | **1.79** | 1.90 | — | **−6 %** ⬇ |

Score vs gs: **5 wins, 4 losses**. sli3 beats nest 2.2–2.9× across
the board. The 4 remaining gs losses (B2b/B7/B8/B9) are the
workloads where gs's threaded-code + packed-ref design has an
intrinsic edge — Axis II step 3 and Axis III territory.

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
- `SLI3_TRACE` block duplicated across both dispatchers — extract
  to one helper.
- `sli_op_bodies.h` — change `static inline` to `inline` to allow
  ODR merging across the 4 TUs that include it.
- Slice 8 step 4 — flip `SLI3_INLINE_BODY_WALK` default to ON
  and delete the OFF path (~330 lines of unreachable code in the
  current ON build). Gate the bench gates listed in
  `doc/axis1_slice8_plan.md` first.

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

### Slice 8 step 4 — flip default + delete OFF path
The body-walk loop already runs under `-DSLI3_INLINE_BODY_WALK=ON`
in benches. The OFF code path is dead in that build, and the ON
code path is dead in the default build. Flipping the default and
deleting the OFF body (`execute_dispatch_` outer cases for
`iiteratetype`/`irepeatetype`/`ifortype`/`iforalltype`) collapses
~330 lines of dispatcher and removes the divergence risk. Gate
behind the bench gates listed in `doc/axis1_slice8_plan.md`.

### Axis II step 3 — tag more hot ops
Mechanical: tag `gt` (B7/B8), `get` (B7/B8), `put` (B7/B8) with
`set_hot_op(...)` and add the matching arms to both dispatcher
hot-op switches (and remember to add to the inline-name switch
too — see CRITICAL #14). Expected B7 / B8 closure: −5 to −10 %.

### Axis III — compact procedure storage
Spec at `doc/compact_procedure_spec.md` §Axis III. Predicted B2b
/ B9 gain ~0.5–1 ns/iter on top of Axis I+II. Depends on Axes I
and II being stable (i.e. slice 8 step 4 flipped).

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

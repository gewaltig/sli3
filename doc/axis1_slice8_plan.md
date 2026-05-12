# Axis I slice 8 — body-walk loop unification

Revision: 2026-05-12. Anchor: commit `3ee5f82` (post-Axis-I-bundle,
docs caught up). Owner: TBD.

This document specifies how to collapse the four iter cases of
`execute_dispatch_` (`iiterate / irepeat / ifor / iforall`) into a
single body-walk loop. It supersedes the slice 8 sketch in
`doc/dispatch_restructure_plan.md` Slice 8 with concrete
implementation steps, a build-flag plan, test gates, and bench
gates per step.

The doc is the result of a failed slice 8.1 attempt (commit
removed before push, see `Lessons` at the bottom): cross-iter-type
resume across the four separate cases required hoisting
`TokenArray* proc; long* pos_p;` to function scope, which cost
+20 % on B3 because the compiler stopped keeping them in
registers across the per-token `fn->execute` call.

The conclusion: the four iter cases can't share state without a
register-allocation penalty so long as they remain four separate
cases. Slice 8 fixes this by unifying them.

---

## Target structure

The four iter cases share a near-identical body-walk loop. The
only differences are:

1. **The iter-marker tag** identifies which iter type the frame
   is. It lives in `pick(0)` of the iter frame.
2. **The body-exhaust handler** runs when `proc->index_is_valid(*pos_p)`
   returns false. Each iter type does something different:
   - `iiterate`: `pop(3)` (drop marker + pos + proc).
   - `irepeat`: if loop counter > 0, reset pos and decrement; else
     `pop(5)` (drop marker + pos + proc + counter + (counter is
     pick(3)? — verify in code)).
   - `ifor`: advance counter by step; if still in range, reset pos
     and push counter; else `pop(7)`.
   - `iforall`: if more array elements, reset pos and push next;
     else `pop(6)`.

Everything ELSE — fetching `proc->get(pos)`, advancing pos,
dispatching the token via fast-path inline (functiontype,
nametype-to-functiontype), pushing literals to ostack — is
identical across the four iter types.

The body-walk hot path runs **once per token in every procedure
body**. For B3 (`100k { 1 1 1k { 2 add pop } for } repeat`),
that's ~600 M iterations: 100k outer × (1k inner × 3 inner-body
tokens). Optimizing this loop is the entire game.

### Proposed dispatcher structure

```cpp
int SLIInterpreter::execute_dispatch_(size_t exitlevel) {
    ...
    try {
      do {
        try {
          while (execution_stack_.load() > exitlevel) {
            ++local_cycles;
            switch (execution_stack_.top().tag()) {
              // ... existing non-iter cases unchanged ...
              case sli3::iiteratetype:
              case sli3::irepeattype:
              case sli3::ifortype:
              case sli3::iforalltype: {
                // Body-walk locals: register-allocatable because
                // they live in the ONE case body that uses them.
                TokenArray *proc = execution_stack_.pick(2).data_.array_val;
                long *pos_p = &execution_stack_.pick(1).data_.long_val;
              body_walk:
                if (proc->index_is_valid(*pos_p)) {
                  const Token &t = proc->get((*pos_p)++);
                  if (not t.is_executable()) {
                    operand_stack_.push(t);
                    if (proc->index_is_valid(*pos_p))
                      goto body_walk;
                    goto body_exhausted;
                  }
                  // ... functiontype / nametype / proceduretype
                  //     fast paths. After fn->execute, check if
                  //     top is any iter marker -- if so, reload
                  //     proc/pos_p and goto body_walk. ...
                }
              body_exhausted:
                // Dispatch by iter type (this is the ONLY place
                // the four types diverge).
                switch (execution_stack_.pick(0).tag()) {
                  case sli3::iiteratetype:
                    execution_stack_.pop(3);
                    break;
                  case sli3::irepeattype: {
                    long &lc = execution_stack_.pick(3).data_.long_val;
                    if (lc > 0) { *pos_p = 0; --lc; }
                    else execution_stack_.pop(5);
                    break;
                  }
                  case sli3::ifortype: {
                    long &count = execution_stack_.pick(3).data_.long_val;
                    long &lim   = execution_stack_.pick(4).data_.long_val;
                    long &inc   = execution_stack_.pick(5).data_.long_val;
                    if (((inc > 0) && (count <= lim))
                     || ((inc < 0) && (count >= lim))) {
                      *pos_p = 0;
                      operand_stack_.push(execution_stack_.pick(3));
                      count += inc;
                    } else execution_stack_.pop(7);
                    break;
                  }
                  case sli3::iforalltype: {
                    TokenArray *ad = execution_stack_.pick(4).data_.array_val;
                    long &idx = execution_stack_.pick(3).data_.long_val;
                    if (ad->index_is_valid(idx)) {
                      *pos_p = 0;
                      operand_stack_.push(ad->get(idx));
                      ++idx;
                    } else execution_stack_.pop(6);
                    break;
                  }
                  default: __builtin_unreachable();
                }
                // After body-exhaust, top may be another iter
                // frame (multi-level body-exit). Re-load and resume.
                switch (execution_stack_.top().tag()) {
                  case sli3::iiteratetype:
                  case sli3::irepeattype:
                  case sli3::ifortype:
                  case sli3::iforalltype:
                    proc  = execution_stack_.pick(2).data_.array_val;
                    pos_p = &execution_stack_.pick(1).data_.long_val;
                    goto body_walk;
                  default: break;
                }
                break;
              }
              // ... existing trailing cases ...
            }
          }
        } catch (...) { ... }
      } while (...);
    } catch (...) { ... }
    ...
}
```

### Why this is faster (vs current four cases)

1. **Multi-level body-exit cascade** at the end of the unified
   case. When an iter body exhausts, if the new top is also an
   iter frame, we re-enter `body_walk` in-place. Today this
   requires an outer-switch round-trip + the case-prelude
   `pick(2)/pick(1)` reload. Saves ~5 ns per cascade step.
   On a 4-deep nested fib, that's 4 cascade steps per return.

2. **Cross-iter-type resume after `fn->execute`** is free.
   Today the iiterate case only resumes if top is iiteratetype;
   any other iter type breaks to outer switch. In the unified
   case, the post-execute check covers all four iter types and
   resumes any of them. This is the win slice 8.1 was after — but
   here it's free because proc/pos_p are case-local (no spill).

3. **Body-walk loop is one tight inner loop**, not four. The
   compiler can specialize it once. Less I-cache pressure.

### Why register allocation works here (vs slice 8.1)

In slice 8.1, `proc` and `pos_p` had to be function-scope to
allow `goto start_repeat` from inside the `case iiteratetype`
block (and vice-versa). The four cases each declared their own
locals; cross-case `goto` had to skip those declarations, which
requires function-scope storage.

In the unified case, there's only ONE case body that walks proc.
`proc / pos_p` are declared inside that body — case-local — and
`goto body_walk` stays within the same scope. The compiler sees
them as register-allocatable just like today's four separate
cases.

The body-exhaust cascade re-loads `proc / pos_p` from the new top
and `goto body_walk` — within the same case, no scope crossing.

---

## Step-by-step plan

The slice lands in three commits behind a build flag, then a
fourth commit removes the flag.

### Pre-step: extend `test_dispatch_parity.cpp`

Today the parity test compares `execute_` (plain mode) against
`execute_dispatch_`. Slice 8 changes `execute_dispatch_` and
must remain parity-clean. Add coverage for:

- Nested procs: `{ { 1 add } exec } exec`.
- Iter types nested across each other: `{ for { repeat { ... } } }`,
  `{ [1 2 3] forall }` inside `for`, etc.
- Stop unwind through 2+ iter frames.
- Error raised inside a 3-deep nested iter.
- Last-ref-is-proc TCO: a 5-call chain of `/f { f } def`-style
  tail recursion (just enough to assert e-stack depth doesn't
  blow up).

This step has no production-code changes; only the test grows.
Acceptance: existing `test_dispatch_parity` plus new cases pass.

### Step 1 — Add build flag, no behavioural change

Add `cmake -DSLI3_INLINE_BODY_WALK=ON|OFF` flag in CMakeLists.txt
that defines `SLI3_INLINE_BODY_WALK_ENABLED` macro (default OFF).
At the start of `execute_dispatch_`, branch on the macro and
dispatch to one of two paths:

```cpp
#ifdef SLI3_INLINE_BODY_WALK_ENABLED
    return execute_dispatch_inline_(exitlevel);
#else
    // existing dispatcher body
#endif
```

`execute_dispatch_inline_` is a separate method, initially a
direct copy of `execute_dispatch_`. The duplication is
temporary; step 4 deletes the old path.

Acceptance:
- CMake configures both modes.
- `cmake -DSLI3_INLINE_BODY_WALK=OFF` builds and `ctest` green.
- `cmake -DSLI3_INLINE_BODY_WALK=ON` builds and `ctest` green
  (using the duplicate, behaviour-identical code).

### Step 2 — Unify the four iter cases under the flag

In `execute_dispatch_inline_` only: collapse the four cases per
the structure above. Add the multi-level body-exit cascade. Add
cross-iter-type resume after `fn->execute`. Do NOT touch the
`OFF` path.

Acceptance:
- `cmake -DSLI3_INLINE_BODY_WALK=ON` builds and `ctest` green.
- `OFF` mode unchanged: ctest still green.
- `test_dispatch_parity` (extended in pre-step) clean.

Bench gate (`ON` vs `OFF`):
- B1, B2, B5: within ±2 % (no iter overhead change).
- B3: ≤ 1.55 s (parity with current 1.52, target ≤ 1.45).
- B2b: ≤ 1.80 s (target ≤ 1.60).
- B7, B8, B9, B10: within ±5 %.

If B3 regresses again, the design needs revisiting — the doc
explains the failure mode; the fix is to reconfirm that `proc /
pos_p` are case-local and the compiler keeps them in registers.

### Step 3 — Optional: extend body-walk to enter nested procs

Currently, when a body contains a `proceduretype` token, we
`execution_stack_.push(t); break;` — the outer switch's
proceduretype case pushes the iiterate frame and falls through.
One outer-switch round-trip.

Under slice 8, we can inline this: when the body-walk sees a
proceduretype token, push `null_val + iiterate_t` directly (the
proceduretype case's work) and `goto body_walk` to walk the new
proc. Saves another ~3 ns per nested proc entry.

This is OPTIONAL within slice 8 — land it only if step 2
bench-gate has headroom. May land as slice 8.5 if step 2 lands
clean but headroom is tight.

Acceptance: same gates as step 2 plus B2b should drop another
~3–5 % from inlining nested entry.

### Step 4 — Flip default, remove the OFF path

After 1 week of `ON` being the default in CI and dev:
- Change the CMake flag default to `ON`.
- After another week: delete `execute_dispatch_` (the old four-
  case version) and the `#ifdef`. Rename
  `execute_dispatch_inline_` to `execute_dispatch_`.
- Delete the build flag.

Acceptance: ctest green; bench green; no `OFF` build path in the
tree.

---

## Risks and mitigations

| Risk | Mitigation |
|---|---|
| Register-allocation regression (slice 8.1 lesson) | Step 2 bench gate explicitly tests this. If `proc / pos_p` get spilled in the unified case, declare them inside a tight inner block so the compiler scopes them aggressively. Worst case: add `__attribute__((hot))` to the function. |
| `backtrace()` overrides on `IiterateFunction` etc. read e-stack with a specific shape | These overrides are in `sli_builtins.cpp:79, 153, 205, 265, 322, 378`. The shape is unchanged — only the dispatcher's *handling* of the shape unifies. Confirm with a `gdb` walk through `backtrace` for each iter type. |
| `cycles` operator counts iterations differently | Today, one outer-switch round-trip per body token = one cycle. With body-walk loop, the inner `goto body_walk` cycles count? Decide: either keep `++local_cycles` once per outer switch iteration (so cycles count drops, possibly user-visible), or `++local_cycles` inside `body_walk` (preserves current count). Recommend: preserve count for parity. |
| `$errordict /estack` snapshot shape | The iter frames look the same; only the inline-execution path changes. Snapshot should be identical. Verify with `test_errors_dispatch`. |
| Stop / stopped unwind through unified iter frames | `StopFunction` walks the e-stack looking for `/istopped`; iter frames are passed transparently. Unaffected by slice 8. Test: existing `test_errors`. |
| TCO behaviour change | Today's iiterate case does in-place proc-slot replacement on last-ref TCO (`pick(2) = t; pop(2)`). The unified case must preserve this for at least iiterate. Optionally extend to irepeat (replace proc, reset pos, keep iter marker) — but this changes counter semantics. Defer extending TCO to irepeat/ifor/iforall until after step 2 is stable. |

---

## Bench-measurement protocol for slice 8

Each step that touches `execute_dispatch_` runs the full bench
suite (`bench/run.sh`) twice — once `ON`, once `OFF` — and
records the deltas. Acceptance gate is **`ON` ≤ `OFF` on every
bench** (no regression) and **`ON` ≤ stage9-complete + 5 %** on
all benches (no surprising blowup).

Best-of-five wall time, AC power, no background load. Always
verify no orphan `sli3` processes after each bench run (the
contamination hazard documented in `doc/next_steps.md`).

---

## Lessons from slice 8.1 attempt (2026-05-12)

I tried a cheaper version of slice 8 — keep the four iter cases
but enable cross-iter-type resume by hoisting `proc / pos_p` to
function scope. Plan: macro `SLI3_RESUME_ITER(EXPECTED_TYPE,
EXPECTED_LABEL)` checks the same-type fast path first, falls
through to a 4-way `SLI3_RESUME_TOP_ITER_FRAME` switch for
cross-type.

Result: B3 regressed +20 % (1.52 → 1.83) — even with the
cross-type fallthrough disabled (verified). The function-scope
hoist alone caused the regression: the compiler stopped keeping
proc/pos_p in registers across `fn->execute` calls and started
spilling them every iteration.

**Diagnostic**: I removed the cross-type slow path, leaving only
the same-type fast path that mirrored the original code. B3
remained at 1.83. So the regression was purely from the function-
scope storage, not from the cross-type switch.

**Conclusion**: cross-iter-type resume can't be done as a
piecemeal change on top of the four-case structure. The
function-scope hoist hurts the hot path more than cross-type
saves. Slice 8 requires the full unification — only then are
proc/pos_p naturally case-local (one case, one place that uses
them).

The slice 8.1 patch was reverted before push. The plan above is
the lesson learned, codified.

---

## File map for slice 8

| What | Path |
|---|---|
| Dispatcher (where the work happens) | `src/interpreter/sli_interpreter.cpp` (function `execute_dispatch_`) |
| Iter operators (push iter frames) | `src/builtins/sli_control.cpp` (LoopFunction, RepeatFunction, ForFunction, Forall*Function) — UNCHANGED |
| Iter continuation funcs (fallback path) | `src/builtins/sli_builtins.cpp` (IiterateFunction, IloopFunction, IforFunction, Iforallarray*Function) — UNCHANGED |
| Parity test | `tests/test_dispatch_parity.cpp` (extend in pre-step) |
| Build flag | `CMakeLists.txt` (add `SLI3_INLINE_BODY_WALK` option) |
| This plan | `doc/axis1_slice8_plan.md` |

---

## Bench expectations (post slice 8 step 2)

Predictions, not measurements:

| Bench | Pre slice 8 | Predicted | Why |
|---|---|---|---|
| B1   | 0.95 | 0.93 | One fewer branch in functiontype outer-switch return |
| B2   | 1.98 | 1.92 | Same |
| B2b  | 1.79 | 1.55 | Inline-nested-proc reduces outer-switch round-trips through `proc` body |
| B3   | 1.52 | 1.42 | Body-exit cascade saves outer-switch for inner for ending |
| B5   | 1.51 | 1.51 | Not iter-heavy; unchanged |
| B7   | 2.10 | 2.00 | Bubble sort has many nested for; cascade helps |
| B8   | 1.47 | 1.42 | Same as B7 |
| B9   | 2.08 | 1.85 | Recursion: each call returns from one proc into another's last ref. Multi-level TCO via cascade. |
| B10  | 1.80 | 1.70 | Nested for in matmul; cascade helps |

If predictions miss by more than 30 %, the design needs
revisiting — but expect noise on individual benches.

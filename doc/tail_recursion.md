# Tail-call optimization: sli3 vs. gs

Both interpreters implement tail-call elision so that
PostScript procedures ending in another procedure call don't
grow the execution stack on each recursion. The two
implementations differ substantially in scope, mechanics, and
when they fire — and the differences explain part of the
B2b-vs-gs perf gap.

## Short version

| | sli3 | gs |
|---|---|---|
| Scope | Only `iiteratetype` (plain proc execution) | Every procedure body, automatically |
| Mechanism | Overwrite the iter frame's `proc` slot with the tail token, then pop `pos` + iter marker | Pop the body frame *before* dispatching the last ref |
| Trigger detection | Check `index_is_valid(pos)` after `pos++` | Built into the loop: `icount` initialized to `size - 1`, so the last ref reaches `out:` instead of `next()` |
| Multi-level TCO | No — one frame elided per tail call | Yes — `up:` keeps popping if successive parents also have 0–1 refs left |
| Loop bodies (`repeat`/`for`/...) | Not elided (frame reused across iterations) | Body frame popped each iteration; continuation op stays on e-stack |
| Configurable | `opt_tailrecursion_` flag exists but is **dead code** — never read | Not configurable; always on |
| Source | `sli_interpreter.cpp:875-881` | `psi/interp.c:1858-1882` (`out:` + `up:`) |

## sli3's TCO

### Where it lives

`SLIInterpreter::execute_dispatch_`, inside the `iiteratetype`
case (`src/interpreter/sli_interpreter.cpp:858-926`). The
relevant arm is lines 875–881:

```cpp
case sli3::iiteratetype: {
    TokenArray *proc = execution_stack_.pick(2).data_.array_val;
start_iterate:
    long &pos = execution_stack_.pick(1).data_.long_val;
    if (proc->index_is_valid(pos)) {
        const Token &t = proc->get(pos++);
        if (not t.is_executable()) { /* push literal, etc. */ }
        if (not proc->index_is_valid(pos)) {       // <-- TCO trigger
            proc->add_reference();
            execution_stack_.pick(2) = t;          // overwrite proc slot
            proc->remove_reference();
            execution_stack_.pop(2);               // drop pos + iiterate
            break;                                  // outer dispatcher dispatches t
        }
        /* ... inline fast paths for functiontype, nametype ... */
        execution_stack_.push(t);
        break;
    }
    execution_stack_.pop(3);   /* empty proc */
    break;
}
```

### How it works

E-stack layout while a procedure is executing
(`iiteratetype` frame, top-down):

```
pick(0) = iiterate marker
pick(1) = pos (integer)
pick(2) = proc (TokenArray ref)
```

When `t = proc->get(pos++)` brings `pos` past the end of the
body, the just-fetched `t` is the last executable token. The
TCO arm:

1. Increments the proc's refcount (it's about to be replaced
   in `pick(2)`; the new ref to it via the `const Token &t`
   needs to survive `remove_reference` on the original slot).
2. Overwrites `pick(2)` with `t`. After this the e-stack is
   `[..., t, pos, iiterate]`.
3. Decrements the proc's refcount (balances step 1).
4. `pop(2)` drops `pos` and the `iiterate` marker. E-stack is
   now `[..., t]`.
5. `break` exits the case; the outer `switch (tag())` re-runs
   on the new top — which is `t`.

The outer dispatcher then dispatches `t` according to its
type. If `t` is a `proceduretype`, `ProcedureType::execute`
(`src/types/sli_arraytype.cpp:134-142`) pushes
`[t_proc, 0, iiterate]`, restoring the depth that was there
before the TCO fired. So a tail-recursive call replaces its
caller's iter frame with the callee's iter frame — net change
in depth: 0.

### Limitations

- **Only `iiteratetype`.** The other iter cases
  (`irepeattype`, `ifortype`, `iforalltype`) do not have this
  optimization. Their frames are reused across iterations —
  `pos` is reset to 0 when the body ends — so the body's last
  token is dispatched while the iter frame is still alive.
- **Strictly one frame.** The optimization elides exactly one
  iter frame per tail call. If the parent frame *also* has
  the current frame as its last element, the parent isn't
  elided too. (This is a real difference from gs; see below.)
- **`opt_tailrecursion_` is dead.** The flag at
  `sli_interpreter.h:568` is initialized to `true`
  (`sli_interpreter.cpp:111`) and toggled by the
  experimentally-disabled debug mode (`sli_control.cpp:1852`),
  but never consulted anywhere in the dispatcher. The TCO
  always fires when the conditions are met. The flag exists
  because it was inherited from NEST 2.x, where it had the
  same dead-code status (NEST 2.20's `interpret.cc:374`
  initializes it; nothing reads it).

### Worked example: tail-recursive procedure

```sli
/loop_fn { ... loop_fn } def    % last token is `loop_fn`
```

Initial call:

```
[..., loop_fn_proc, 0, iiterate]    e-stack depth = D + 3
```

Walk the body. When the last token is fetched (`pos`
incremented past the end):

- TCO fires: `pick(2) = loop_fn_name_token`; `pop(2)`.
- E-stack: `[..., loop_fn_name_token]` (depth = D + 1).

Outer dispatch: name → lookup → `loop_fn` procedure. Push.
`ProcedureType::execute` pushes `[0, iiterate]`. E-stack:

```
[..., loop_fn_proc, 0, iiterate]    e-stack depth = D + 3
```

Same depth as the initial call. The recursion can run
indefinitely without overflow.

### Worked example: non-tail call

```sli
/inner { 1 } def
/outer { inner pop } def
```

Calling `outer`:

- Enter outer: `[..., outer_proc, 0, iiterate]`.
- pos=0: fetch `inner` (name). pos++ → 1. `index_is_valid(1)` is
  true (size=2), so TCO does *not* fire. Inline name
  dispatch resolves `inner` to its proc; push it onto e-stack:
  `[..., outer_proc, 1, iiterate, inner_proc_token]`.
- ProcedureType::execute on `inner_proc_token` pushes
  `[0, iiterate]`:
  `[..., outer_proc, 1, iiterate, inner_proc_token, 0, iiterate]`.
- inner's body: fetch literal `1`, push to ostack, pos++ → 1,
  `index_is_valid(1)` false → `pop(3)` (the empty-tail path,
  line 870). E-stack:
  `[..., outer_proc, 1, iiterate]`. Back in outer.
- pos=1: fetch `pop` (name). pos++ → 2, `index_is_valid(2)`
  false → TCO fires. E-stack: `[..., pop_name_token]`.
- Outer dispatch: name → `pop` (functiontype). Push, execute,
  self-pop. E-stack: `[...]`.

Even in this two-token outer body, the TCO at the *outer's
own tail call* keeps the e-stack tight. The inner call wasn't
in tail position (a `pop` followed it), so it didn't TCO; it
grew the e-stack and shrank it again normally.

## gs's TCO

### Where it lives

`psi/interp.c:1858-1882`, the `bot:` / `out:` / `up:` triad
at the end of the main dispatcher loop:

```c
bot: next();                          /* advance & dispatch */
out:                                  /* fell here when icount went to 0 */
    if (!icount) {
        iesp--;                       /* pop the body frame */
        iref_packed = IREF_NEXT(iref_packed);   /* advance to last ref */
        goto top;                     /* dispatch the last ref */
    }
up: if (--(*ticks_left) < 0) goto slice;
    if (!r_is_proc(iesp)) {           /* continuation op or other */
        SET_IREF(iesp--);
        icount = 0;
        goto top;
    }
    SET_IREF(iesp->value.refs);
    icount = r_size(iesp) - 1;
    if (icount <= 0) {                /* parent has 0–1 refs left */
        iesp--;                       /* pop, or further tail recursion */
        if (icount < 0) goto up;
    }
    goto top;
```

### How it works

gs's TCO is structural — it follows from how `interp()`
initializes proc-body iteration:

- At proc entry (`prst:` / `pr:`,
  `psi/interp.c:1339-1361`), the dispatcher computes
  `icount = r_size(pvalue) - 1`. This intentionally *excludes
  the last ref*: only `icount` remaining-after-the-first refs
  are tracked.
- The `next()` macro (`psi/interp.c:1076-1077`):
  ```c
  #define next()\
    if (--icount > 0) { iref_packed = IREF_NEXT(iref_packed); goto top; }\
    else goto out
  ```
- After dispatching ref[k], `next()` decrements `icount`. When
  `icount` goes 1 → 0 (i.e. we just dispatched the
  second-to-last ref), control falls to `out:`.
- `out:` pops the body frame (`iesp--`), advances `iref` to
  point at the actual last ref, and `goto top` dispatches it.
  **The last ref of every proc body is dispatched after the
  body frame has already been popped.**

If that last ref is a procedure call, the dispatcher enters
the nested proc by pushing a new frame onto `iesp` — but the
slot is the same one the current frame just vacated. No
e-stack growth across the tail call.

If the last ref is a literal or non-proc operator, it's
dispatched normally; control then flows via `bot:` →
`next()` → (icount is undefined here — actually let me trace
this more carefully).

### Worked example: 4-element body

Body `[r0 r1 r2 r3]`, `r_size = 4`. Entry at `prst:`:

```
icount = r_size - 1 = 3
iesp++ ; iesp->value.refs = &r0 ; iref = &r0
```

Iteration:

| step | dispatched | next() does | iref after | icount after |
|---|---|---|---|---|
| 1 | r0 | --icount=2, advance | &r1 | 2 |
| 2 | r1 | --icount=1, advance | &r2 | 1 |
| 3 | r2 | --icount=0, **goto out** | &r2 | 0 |

At `out:` (after step 3):

```c
iesp--;                          /* pop this body frame */
iref = IREF_NEXT(iref);          /* iref → &r3 */
goto top;                        /* dispatch r3 */
```

So r3 (the *last* element) is dispatched with the body frame
already popped.

After dispatching r3:
- If r3 was a literal/operator returning normally: control
  flows to `next()` via `bot:`. `--icount` makes icount go
  from 0 to -1; condition `--icount > 0` is false; `goto out:`.
- At `out:`: `if (!icount)` — icount is -1, false; fall
  through to `up:`.
- `up:` loads the parent body's saved state from `iesp` and
  continues there.

If r3 was a procedure-entering op (`pr:`), it pushes a new
frame and the new body's iteration begins. The parent's frame
(the one that called this proc) is one slot down; when this
new proc ends, `out:` pops it, and `up:` resumes the parent.

### Multi-level TCO at `up:`

Once `out:` pops the just-finished frame, `up:` examines what
the new top of `iesp` holds:

```c
SET_IREF(iesp->value.refs);
icount = r_size(iesp) - 1;
if (icount <= 0) {                  /* parent has 0 or 1 refs left */
    iesp--;                         /* pop, or further tail recursion */
    if (icount < 0) goto up;        /* parent was empty; recurse */
}
goto top;
```

If the parent body has 0 or 1 ref left, gs pops it too,
recursively. A chain of procedures that all end in
procedure calls can collapse arbitrarily many frames in one
`out:` → `up:` → `up:` cascade.

### Loop bodies

gs's loop continuation ops (`repeat_continue`, `for_pos_int_
continue`, `loop_continue`, …) are operator refs that sit
permanently on `iesp` between iterations. The proc body is a
*separate frame* pushed above the continuation op for each
iteration, popped at body end (via `out:`). Sequence:

```
[..., mark, count, proc, repeat_continue]     /* loop setup */
[..., mark, count, proc, repeat_continue, proc_frame]  /* iter N */
[..., mark, count, proc, repeat_continue]     /* after out: pops proc_frame */
                                              /* up: dispatches repeat_continue */
                                              /* it pushes another proc copy */
[..., mark, count, proc, repeat_continue, proc_frame]  /* iter N+1 */
...
```

No e-stack growth across loop iterations: each iteration is
push + walk + pop, returning to the same continuation-op
slot.

## Direct comparison

### 1. Scope and reach

- **sli3** elides one frame per tail call, only inside
  `iiteratetype`. Procedure tail-recursion works; tail-
  recursive operator chains across loop iterations don't get
  TCO (the loop frame isn't elided, but it doesn't need to be
  — the loop reuses one frame).
- **gs** elides one frame at the end of every procedure body
  unconditionally. The mechanism is structural: every body's
  last ref is dispatched after the body frame is popped. And
  `up:` cascades further when parents are also about to
  finish.

### 2. How "tail position" is detected

- **sli3** detects it dynamically: after `pos++`, the test
  `not proc->index_is_valid(pos)` decides. This is a runtime
  check.
- **gs** detects it structurally: `icount` is initialized to
  `r_size - 1`, so `next()`'s post-dispatch arithmetic
  naturally lands the last ref at `out:`. No explicit check.

### 3. Frame mutation vs frame popping

- **sli3** overwrites the proc slot in place
  (`pick(2) = t`), then pops the two slots above. The
  refcount dance (`add_reference` / `remove_reference`
  around the overwrite) preserves the TokenArray pointer in
  the temporary `const Token &t` reference.
- **gs** simply decrements `iesp` and re-uses the dispatcher's
  register state. No refcount work — gs's refs are
  GC-managed; there is no per-ref refcount.

### 4. Behaviour with loop bodies

- **sli3** keeps the loop's iter frame
  (`irepeattype`/`ifortype`/`iforalltype`) across iterations.
  The body's tail token is dispatched normally; no TCO.
  Tail-call elision *across iterations* isn't needed because
  the same frame is reused.
- **gs** pushes and pops a fresh proc-body frame for each
  iteration above the persistent continuation operator. The
  body's tail token triggers `out:` which pops the body
  frame; `up:` dispatches the continuation op which pushes
  again.

Functionally equivalent; structurally different. sli3's
approach is simpler (one frame); gs's gives a uniform shape
("always walk a body, then pop"). gs's approach composes
better with the rest of the dispatcher because there are no
special "iter-type marker" cases to handle.

### 5. The dead `opt_tailrecursion_` flag

`SLIInterpreter::opt_tailrecursion_` (`sli_interpreter.h:568`)
is the historical remnant of a feature that the NEST 2.x
codebase never finished implementing. In sli3 it is:

- declared, default `true` (`sli_interpreter.cpp:111`);
- never read by any code path;
- referenced in a help string for an experimentally-disabled
  debug mode (`sli_control.cpp:1852`).

In NEST 2.20.2's `sli/interpret.cc`, the same story holds:
the flag is initialized at `:374`, set to `false` by
`backtrace_on` (`:997`) and back to `true` by `backtrace_off`
(`:1006`), and toggleable in the debugger UI (`:1166-1169`).
Nothing in `execute_(.)` ever consults it. The flag predates
the current TCO implementation and was probably meant to gate
it; somewhere along the way the gate got optimized out and
nobody removed the flag.

**Action item for sli3:** delete `opt_tailrecursion_` and the
referenced help-text line. Dead code; no behavior change.

## Why the difference matters for performance

The B2b workload (`{1 1 add pop} bind repeat × 100M`) does
not exercise TCO at all — the procedure body has no tail
call (it ends in `pop` which is an operator, not a proc
call). Both sli3's and gs's TCO are dormant on this
benchmark.

Where it matters:

- **Tail-recursive SLI procedures.** sli3 handles them
  correctly thanks to the `iiteratetype` TCO. No regression
  risk if we keep that arm in the rewrite.
- **Deep proc-call chains.** gs's multi-level `up:` cascade
  collapses chains in one cycle of dispatch overhead. sli3
  needs N dispatcher iterations to collapse N frames (one
  per `case iiteratetype` re-entry that finds itself
  immediately at end-of-body). Not common on the benches,
  but matters for SLI scripts with many small helper procs.

## Implications for the Axis I dispatch restructure

The dispatch-restructure plan (`doc/dispatch_restructure_plan.md`)
needs to preserve TCO. Slice 8 (the structural rewrite) is
where this lands. Two design choices:

**(a)** Keep sli3's "overwrite frame, pop 2" mechanic in the
new dispatcher. Cheap to port; only one body-end form to
handle. Doesn't get gs's multi-level cascade.

**(b)** Adopt gs's `out:`/`up:` topology. Initialize
`icount = body_size - 1` on body entry; `next()` advances
icount, falling to `out:` after the second-to-last dispatch;
`out:` pops the frame and dispatches the last ref with the
frame already popped; `up:` cascades further.

Recommendation: **(b)**. It's the same code complexity but
gives the multi-level cascade for free, and it matches the
mental model of gs's interpreter so the rest of the slice 8
work (proc-body walking, continuation-op handling) flows
naturally. The cost is a slight change to how iter operators
(`zrepeat`-equivalents) set up their continuation: they need
to push a real continuation operator on top of the proc
ref, not an iter-type marker.

If we go with (b), this should be a separate sub-slice
inside slice 8: first land the `next()`/`out:`/`up:` topology
with one iter type still using the marker, verify the bench
deltas, then convert iter operators to continuation operators
in a follow-up.

## Reference

| What | sli3 | gs |
|---|---|---|
| Dispatcher entry to proc body | `sli_arraytype.cpp:134-142` (`ProcedureType::execute`) | `psi/interp.c:1339-1361` (`prst:`/`pr:`) |
| Body iteration | `sli_interpreter.cpp:858-926` (`iiteratetype` case) | `psi/interp.c:1100-1156` (main switch) + `next()` macro at `:1076-1077` |
| TCO trigger | `sli_interpreter.cpp:875-881` | `psi/interp.c:1858-1866` (`out:`) |
| Multi-level cascade | — (not implemented) | `psi/interp.c:1867-1882` (`up:`) |
| Loop body end | `sli_interpreter.cpp:991-998` (reset pos / dec counter) | `psi/zcontrol.c:486-498` (`repeat_continue` decides) |
| TCO flag | `sli_interpreter.h:568` (`opt_tailrecursion_`, dead) | none |

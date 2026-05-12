# sli3 control-flow execution contracts

Revision: 2026-05-12. Status: **draft for review**. Anchor:
commit `7948d58` (slice 8 plan landed).

This document specifies the contract that the sli3 dispatcher and
the C++ control-flow operators jointly implement. It is the
authoritative description of:

- The e-stack frame layout of each control structure.
- How a procedure body is walked (including TCO).
- How nested loops compose.
- How `exit`, `stop`, and `stopped` unwind the e-stack.
- How `raiseerror` interacts with stack recording (`recordstacks`).
- Where sli3 follows PostScript Level 2 (the Red Book) and where it
  intentionally diverges.

This is the **contract** that slice 8 of Axis I must preserve. Any
change to the dispatcher or the iter setup ops must be checked
against the invariants below. Tests in
`tests/test_dispatch_parity.cpp`,
`tests/test_errors_dispatch.cpp`, and `tests/test_state_ops.cpp`
together cover the invariants; new tests should be added to those
files when the surface grows.

References used while writing this spec:
- The PostScript Language Reference, Third Edition ("Red Book"),
  sections 3.5 (Procedures), 3.10 (Errors), 3.11 (Execution),
  3.7.1 (Operand stack management).
- Ghostscript reference (`doc/gs_reference.md` §6 — continuation
  operators).
- Tail-recursion comparison (`doc/tail_recursion.md`).
- Existing sli3 implementation as of `7948d58`.

---

## 1. Scope, terminology, and notation

**Scope.** The spec covers `proceduretype` execution, the four
"iter marker" control structures (`iiterate`, `irepeat`, `ifor`,
`iforall`), the function-style continuation operator `iloop`, the
unwind operators (`exit`, `stop`, `stopped`), and error machinery
(`raiseerror` and the `recordstacks` snapshot). The Map family
(`Map`, `MapIndexed`, `MapThread` and their `::Map` continuation
ops) is covered by an appendix because it is built on the same
primitives.

**Three stacks.** The interpreter has three independent stacks:
- *Operand stack* (`OStack` / `operand_stack_`): user data, results.
- *Execution stack* (`EStack` / `execution_stack_`): the program
  to execute. The dispatcher reads its top each cycle.
- *Dictionary stack* (`DStack` / `dictionary_stack_`): name
  resolution context.

This spec talks about the *execution stack* unless noted otherwise.

**Frame notation.** E-stack frames are drawn with top on the left:

    [ marker | pos | proc | ... | caller ]
      top                            bottom

Square brackets denote a complete iter frame. `marker` is a
type-tagged token recognized by the dispatcher's main switch.
`pos` and `proc` are integer and proceduretype tokens. The
"`...`" placeholder is iter-specific state (loop counter, limit,
increment, array, idx — see §2).

**Dispatcher contract.** The dispatcher runs in a loop:

    while EStack non-empty and depth > exitlevel:
        switch on EStack.top().tag():
          case data type:        push to OStack; pop EStack
          case nametype:         resolve via dstack; replace top
          case functiontype:     pre-pop slot (new-ABI) or self-pop (old-ABI); call fn
          case proceduretype:    push [pos=0, iiterate-marker]; fall to iiterate
          case iiteratetype:     walk body (see §2.1, §3)
          case irepeattype:      same shape, different body-exhaust (§2.3)
          case ifortype:         same shape (§2.4)
          case iforalltype:      same shape (§2.5)
          case trietype:         look up in trie via operand top types
          case quittype:         exit dispatcher loop

The four iter-marker cases share the body-walk hot path. Slice 8
unifies them; the spec describes the **contract** the unified
case must implement, not a specific code layout.

---

## 2. E-stack frame catalogue

This section is the authoritative layout for every iter frame.
Any divergence between code and this section is a bug; fix the
code or update this section first.

### 2.1 Procedure execution — `iiterate`

A procedure on the e-stack auto-converts into an iter frame on
dispatch. The dispatcher's `case sli3::proceduretype:` push-
through is:

    Before:                   After fall-through to iiterate:

    [ proc | ... ]            [ iiterate | pos=0 | proc | ... ]
      top                       top

**Frame depth: 3** (`pop(3)` at end of body).

| pick | type           | contents               |
|------|----------------|------------------------|
| 0    | iiteratetype   | marker (replaces nothing — pushed) |
| 1    | integertype    | pos (starts at 0, incremented per token) |
| 2    | proceduretype  | the body being walked (was top before fall-through) |

**Invariants:**

- `pos` is in `[0, proc.size()]`. The dispatcher checks
  `proc->index_is_valid(pos)` before each fetch.
- `proc` is held with a refcount; the e-stack slot owns one
  reference. Body walking does NOT increment-then-decrement on
  every iteration — the slot's refcount carries the lifetime.
- The body-walk loop reads `proc->get(pos++)` and dispatches the
  token. Non-executable tokens go to the operand stack;
  executable tokens are handled per their type (see §3).

**Exit paths:**

- *Normal completion* (pos reaches end): `pop(3)`.
- *Last-ref TCO* (the next-to-last fetch and the body is about
  to exhaust): replace `pick(2)` with the last token; `pop(2)`.
  The remaining `last_token` is what the parent dispatch handles.
  Used today only by the iiterate case. See §3.3.
- *Exit / stop / error*: see §6, §7, §8.

### 2.2 Loop — `iloop` (continuation-op style)

`loop` is NOT an iter-marker case; it's a function dispatched as
ordinary functiontype on the e-stack. The function's body walks
proc and re-runs itself when the body exhausts.

**Setup (LoopFunction):**

    Before:                   After LoopFunction:

    [ proc | ... ]            [ %loop | pos=0 | proc | mark | ... ]
      top  (ostack)             top

The `proc` argument was on the **operand** stack; it moves to
the e-stack frame. The mark below the frame is the unwind target
for `exit`.

**Frame depth: 4** (`pop(4)` would clear it; `exit` finds the
mark instead).

| pick | type           | contents               |
|------|----------------|------------------------|
| 0    | functiontype   | `::loop` (iloopfunction) |
| 1    | integertype    | pos |
| 2    | proceduretype  | the body |
| 3    | marktype       | unwind target for `exit` |

**Per-iteration behaviour (IloopFunction::execute):**

Walks `proc` from `pos` forward. For each non-executable token,
push to ostack. For the first executable token, push it on top
of the e-stack (the dispatcher will execute it next) and return
from IloopFunction. Because IloopFunction never popped itself, it
stays at `pick(0)+1` (now `pick(1)` after the executable push);
when the dispatcher finishes the executable, it returns to the
frame and IloopFunction is re-invoked.

When pos reaches end of proc: reset `pos = 0` and return.
IloopFunction re-runs from the start — infinite loop until
`exit`, `stop`, or an error unwinds the frame.

**Exit paths:**

- *Normal completion*: there is none. `loop` runs until
  interrupted.
- *Exit*: ExitFunction walks the e-stack to find `mark`, pops
  all frames above and including the mark.
- *Stop / error*: §7, §8.

### 2.3 Repeat — `irepeat`

**Setup (RepeatFunction):**

    Before (OStack):   [ count | proc | ... ]
    Before (EStack):   [ ... ]
    
    After RepeatFunction:
    EStack: [ irepeat | pos=0 | proc | count | mark | ... ]
            top                                       bottom-of-frame

`proc` and `count` move from operand stack to e-stack frame.

**Frame depth: 5** (`pop(5)` at end).

| pick | type           | contents               |
|------|----------------|------------------------|
| 0    | irepeattype    | marker |
| 1    | integertype    | pos in current iteration |
| 2    | proceduretype  | the body |
| 3    | integertype    | remaining loop count |
| 4    | marktype       | unwind target for `exit` |

**Body-exhaust handler:**

    if (loop_count > 0) {
        pos = 0;          // reset for next iteration
        --loop_count;
    } else {
        pop(5);           // frame done
    }

**Invariants:**

- `loop_count` is the number of iterations REMAINING (decremented
  each time pos resets to 0). `count = 0` after the last body
  iteration completes.
- The mark at `pick(4)` is the unwind target for `exit`; `exit`
  pops `[irepeat, pos, proc, count, mark]` (five frames).
- The body's operand-stack contract is documented by the user —
  `repeat` does NOT push a counter to ostack like `for` does.

### 2.4 For — `ifor`

**Setup (ForFunction):**

    Before (OStack): [ counter | limit | incr | proc | ... ]
    Before (EStack): [ ... ]
    
    After ForFunction:
    EStack: [ ifor | pos=0 | proc | counter | limit | incr | mark | ... ]

`counter`, `limit`, `incr`, `proc` move from operand to e-stack.

**Frame depth: 7** (`pop(7)` at end).

| pick | type           | contents               |
|------|----------------|------------------------|
| 0    | ifortype       | marker |
| 1    | integertype    | pos |
| 2    | proceduretype  | body |
| 3    | integertype    | current counter (changes each iteration) |
| 4    | integertype    | limit (unchanged) |
| 5    | integertype    | increment (unchanged) |
| 6    | marktype       | unwind target |

**Body-exhaust handler:**

    if ((incr > 0 && counter <= limit) ||
        (incr < 0 && counter >= limit)) {
        pos = 0;
        OStack.push(counter);  // counter visible to body
        counter += incr;
    } else {
        pop(7);
    }

**Invariants:**

- The counter is **pushed to the operand stack at the start of
  each body iteration** — before the body runs. The body sees
  the current counter on the operand stack top.
- `incr == 0` is undefined behaviour (infinite loop with constant
  counter); the spec does not require a check (matches Red Book).
- Body completion: counter has already been advanced for the next
  iteration when body-exhaust fires.

### 2.5 Forall (array) — `iforall`

**Setup (Forall_aFunction):**

    Before (OStack): [ array | proc | ... ]
    
    After Forall_aFunction:
    EStack: [ iforall | pos=0 | proc | idx=0 | array | mark | ... ]

**Frame depth: 6** (`pop(6)` at end).

| pick | type           | contents               |
|------|----------------|------------------------|
| 0    | iforalltype    | marker |
| 1    | integertype    | pos in body |
| 2    | proceduretype  | body |
| 3    | integertype    | current array index |
| 4    | arraytype      | the array being iterated |
| 5    | marktype       | unwind target |

**Body-exhaust handler:**

    if (idx < array.size()) {
        pos = 0;
        OStack.push(array[idx]);
        ++idx;
    } else {
        pop(6);
    }

### 2.6 Forall_s and Forallindexed_a/s

`forall_s`, `forallindexed_a`, and `forallindexed_s` are
**function-style** continuation operators (like `iloop`), not
marker tokens. Their frames look like §2.2 (`%forall_s` /
`%forallindexed_*` is a functiontype Token on top of the frame).
They are not dispatched in the unified iter case; they're
ordinary functiontype dispatches. Slice 8 does not change them.

### 2.7 Map family

`Map`, `MapIndexed`, `MapThread` push their continuation operator
(`::Map`, `::MapIndexed`, `::MapThread`) onto the e-stack
following the same continuation-op pattern as `iloop`. Frame
layout (`::Map`):

    [ ::Map | pos | proc | result | mark | ... ]

Detail: see `src/builtins/sli_array_module.cpp`. Slice 8 does not
touch the Map family.

---

## 3. Procedure execution contract

### 3.1 Entry

A `proceduretype` token reaching the dispatcher's main switch
enters body-walk via the proceduretype case:

    push integer 0 (pos)
    push iiterate marker (becomes new top)
    fall through to iiterate case

Equivalent in functional terms: "convert the proc Token on top
into a 3-slot iter frame whose marker says 'walk this body'."

### 3.2 Body walking — per-token semantics

For each token `t = proc[pos++]`:

| `t.tag()`            | Action |
|----------------------|--------|
| integertype, doubletype, booltype, literaltype, marktype, stringtype, arraytype, dictionarytype | Push `t` onto OStack. |
| nametype             | Resolve via dictstack. If result is executable, replace top of e-stack with result and re-dispatch. Else push to OStack. |
| functiontype         | Dispatcher calls `fn->execute(this)`. Pre-pop or self-pop per ABI (Axis I bundle). |
| trietype             | Look up in trie using current OStack top types. If trie returns executable, replace top; else push to OStack. |
| proceduretype        | Push a new 3-slot iter frame for the nested proc; continue from the inner pos=0. |
| litproceduretype     | Tag-only variant of arraytype — pushed to OStack as-is. |
| iiterate/irepeat/ifor/iforall marker | Cannot appear in a normal proc body. Iter markers are pushed by their setup ops, not by user code. If one appears, the dispatcher will treat it as a frame top — pathological. |

### 3.3 Tail-call optimization (TCO)

When the last token of the body is being fetched (i.e., after
the fetch the body is exhausted), the dispatcher MAY replace the
proc slot with the fetched token in place and pop the marker +
pos:

    Before fetch:  [ iiterate | pos=N-1 | proc | ... ]
    Fetch t = proc[N-1]; pos = N
    Replace proc slot with t; pop (iiterate, pos):
    After:         [ t | ... ]

If `t` is itself a proceduretype, the next dispatcher iteration
pushes a new iter frame for it — at the same depth where the
old frame was. The e-stack does NOT grow per tail call.

**Today's scope**: iiterate implements this; irepeat/ifor/iforall
do not (they need to preserve their per-frame counters across
iterations, so single-level TCO doesn't apply). When slice 8
unifies the cases, single-level TCO for iiterate continues to
work; multi-level TCO (a body that ends with a proc that ends
with a proc) falls out of the unified cascade if the
implementation cascades the body-exit pop (see §4.2).

**PS conformance**: Red Book §3.5 ("Procedures … the language
guarantees that tail recursion does not consume execution-stack
space"). sli3 conforms for proceduretype-only chains. Loops do
not tail-recurse on body exhaustion (because the frame must
persist for the next iteration).

### 3.4 Normal completion

When `pos == proc.size()`:

- iiterate: `pop(3)`. No operand-stack residue from the frame
  itself. Whatever the body pushed is the result.
- irepeat: see §2.3 — either reset for next iteration or
  `pop(5)`.
- ifor / iforall: similar — either reset and push counter, or
  pop the full frame.

After the frame pops, the dispatcher resumes from the new top
(which may itself be an iter frame — see §4.2).

---

## 4. Nested loops

### 4.1 Frame stacking discipline

Iter frames stack naturally. Inner loop's frame sits on top of
the outer loop's frame. The body of the outer loop, when it
calls the inner loop's setup op (e.g., `for`), pushes the inner
frame; control then walks the inner body.

Example: `1 1 3 { 2 { foo } repeat } for`

After the outer `for` setup:

    [ ifor | pos=0 | for-body | ctr=1 | lim=3 | inc=1 | mark | ... ]

Outer body iteration: counter pushed to ostack, then body
`{ 2 { foo } repeat }` runs. After repeat's setup:

    [ irepeat | pos=0 | repeat-body | count=2 | mark
    | ifor   | pos=N | for-body    | ctr=2   | lim=3 | inc=1 | mark | ... ]
      top                                                              bottom

(N = position in the for-body just past the repeat call.)

The dispatcher walks the inner irepeat frame to completion (or
exit/stop). On normal exhaust, the inner frame pops; the outer
ifor frame resumes from pos N+1.

### 4.2 Multi-level body exit

When an inner frame finishes (body-exhaust pop) AND the new top
is another iter frame, the dispatcher SHOULD resume the outer
frame's body-walk directly — no outer-switch round trip.

**Today's behaviour** (pre slice 8): the inner case body
exhausts, `pop(N); break;`. The outer dispatch loop's switch
sees the outer iter marker on top, re-enters the appropriate
case, re-loads `proc`/`pos_p` from `pick(2)`/`pick(1)`, and
continues. One outer-switch round-trip per cascade step.

**Slice 8 behaviour**: the unified iter case checks the new top
after each body-exhaust; if it's an iter marker, re-load
`proc`/`pos_p` and `goto body_walk` directly. No round-trip.

**Invariant preserved**: from the operator's standpoint, the
order of effects is identical — the same operands are pushed in
the same order. The only observable change is timing.

### 4.3 Operands surviving body completion

Operands pushed by the body during the last iteration **survive
body completion**. The frame teardown only touches the e-stack,
not the o-stack. This is part of every iter type's contract.

Example: `[1 2 3] { } forall` leaves `[1, 2, 3]` on the operand
stack after the frame completes — the body's `dup`-style
pushed values stay.

For `for`: the counter pushed by the body-exhaust handler before
the FINAL body iteration also stays on the ostack after the loop
completes, **except** when the loop terminates because the
bounds check fails — in that case no counter was pushed for that
"would-be" iteration. The body sees the counter before each
body run.

---

## 5. Exit semantics

### 5.1 `exit` operator

`exit` walks the e-stack from itself downward looking for the
nearest `marktype` slot. When found, pops all e-stack frames
from `exit`'s own slot through the mark inclusive.

Pseudocode (matches ExitFunction):

    n = 1     # start at pick(1), skip exit's own slot at pick(0)
    while pick(n) not marktype and n < load:
        ++n
    if not found: raiseerror("EStackUnderflow"); return
    pop(n + 1)   # includes exit itself + everything up through mark

**Frames that have a mark** (eligible exit targets):

- `loop` (pick(3) of its frame).
- `repeat` (pick(4) of its frame).
- `for` (pick(6) of its frame).
- `forall_a` (pick(5) of its frame).
- `forall_s`, `forallindexed_*` (continuation-op pattern — also push a mark).
- Map family.

`iiterate` (procedure execution) does NOT push a mark. `exit`
inside a procedure body that is itself inside a loop unwinds
PAST the procedure's iiterate frame to the enclosing loop's
mark. This matches PS standard ("exit terminates the innermost
enclosing looping context").

### 5.2 PostScript conformance

Red Book §3.11: "If there is no enclosing loop, exit issues an
`invalidexit` error." sli3 raises `EStackUnderflow` — different
name, same effect (the program is interrupted). The error
machinery (§8) catches this if a `stopped` context is present.

### 5.3 Inner-most enclosing context

Stop ≠ exit: `stop` walks for `::stopped` (§7), `exit` walks for
`marktype`. A `stopped` context does NOT contain a mark; it
contains the literal name `::stopped`. So:

- `exit` does not break out of `stopped`. It breaks out of the
  innermost enclosing **loop** (looking for mark).
- `stop` does not break out of a loop unless the loop is itself
  inside a `stopped`. It breaks out of the innermost enclosing
  **stopped** context.

These differ from gs's structure but match the Red Book's
description of `exit` vs. `stop`.

---

## 6. Stop / stopped contract

### 6.1 PostScript semantics

Red Book §3.10: `stop` raises a signal; the nearest dynamically-
enclosing `stopped` context catches it and pushes `true` on the
operand stack. If no `stopped` context exists, the interpreter
behaviour is implementation-defined (gs prints a message and
returns to the REPL).

### 6.2 sli3 implementation

**StoppedFunction** pushes a frame:

    Before:  EStack [ ... ]   OStack [ proc | ... ]
    After:   EStack [ proc | ::stopped | ... ]   OStack [ ... ]

The frame is just `[proc, ::stopped]` — two slots. The proc is
pushed on top so it dispatches first. After it completes
normally, ::stopped (a literal name) is on top; the dispatcher
resolves it via the dict stack. ::stopped is bound to `false`
(see `sli_control.cpp:2029`), so the resolution pushes `false`
to the operand stack — the "no stop was raised" signal.

**StopFunction** walks the e-stack looking for the literal name
`::stopped`:

    n = 1
    while pick(n) != ::stopped and n < load:
        ++n
    if !found:
        message(M_FATAL, "stop", "No stopped context");
        EStack.clear()
        return
    OStack.push(true)
    EStack.pop(n + 1)   # includes stop + frames up through ::stopped

Note: stop's pop INCLUDES the ::stopped marker. Operand stack
gets `true` pushed after the unwind.

### 6.3 Interaction with iter frames

When `stop` is raised inside a deeply nested context, all iter
frames between stop and the nearest ::stopped are popped
**without their body-exhaust handlers running**. The frames are
simply discarded.

Example: `{ { 5 { stop } repeat } stopped } exec`

E-stack at the moment `stop` runs:

    [ stop                          # being dispatched
    | irepeat | pos | proc | count=4 | mark
    | iiterate | pos | proc          # outer proc (`{ 5 { ... } repeat }`)
    | ::stopped
    | iiterate | pos | proc          # outer exec's proc
    | ... ]

StopFunction finds ::stopped at pick(7), `pop(8)`:

    [ iiterate | pos | proc | ... ]
    OStack: [ true ]

The repeat frame's mark, count, etc. are all gone — no
body-exhaust ran. This is correct: the repeat was abandoned.

**Invariant**: the dictionary stack is NOT unwound by stop. PS
standard says the dictionary stack is automatically restored to
the depth it had at the `stopped` call — but sli3 does not
implement this today (a known PS-conformance gap; see §10).

### 6.4 stop without stopped → fatal

When no ::stopped is on the e-stack and stop is raised, sli3
prints a fatal message and clears the e-stack:

    "stop [M_FATAL]: No stopped context was found!"

This matches Red Book §3.10 ("implementation-defined behaviour").
The interpreter does not exit; the REPL continues. gs prints
similarly and continues.

---

## 7. raiseerror and recordstacks

### 7.1 The two-stage error path

sli3's error path:

1. C++ code (operator body, dispatcher catch handler, library
   code) calls `i->raiseerror(N)` or `i->raiseerror(N, N)`.
2. raiseerror populates `errordict_` and pushes the `stop`
   operator onto the execution stack.
3. The next dispatcher iteration runs `stop`, which unwinds to
   the nearest stopped frame and pushes `true`.
4. The stopped context's caller checks the bool and dispatches
   the user's error handler (typically `handleerror` in
   sli-init.sli).

### 7.2 errordict shape

After `raiseerror(cmd, err)`:

| key             | type        | content |
|-----------------|-------------|---------|
| `newerror`      | booltype    | true |
| `errorname`     | literaltype | `err` (e.g., `/StackUnderflow`) |
| `commandname`   | literaltype | `cmd` (the operator that raised) |
| `command`       | (varies)    | The failing token (set in `raiseerror(exception&)`; not always set in `raiseerror(Name)`/`raiseerror(Name,Name)`) |
| `message`       | stringtype  | exc.what() (set in `raiseerror(exception&)`) |

If `recordstacks == true` (the default after sli-init.sli runs
`init_errordict`):

| key             | type        | content |
|-----------------|-------------|---------|
| `ostack`        | arraytype   | snapshot of OStack at error time |
| `estack`        | arraytype   | snapshot of EStack at error time |
| `dstack`        | arraytype   | snapshot of DStack at error time |
| `oldcommandname`| literaltype | previous `commandname` |
| `olderrorname`  | literaltype | previous `errorname` |
| `oldestack`     | arraytype   | previous `estack` snapshot |

### 7.3 Stack-recording timing

`recordstacks` snapshots are taken **after** raiseerror has
popped the calling operator (under old ABI) or after the
dispatcher has pre-popped the calling fn slot (under new ABI).
The snapshot therefore reflects the state "as if the failing
operator had just returned and `stop` were about to fire" —
matching the gs semantics: the operator's own slot is gone, but
the surrounding context is intact.

This is important for `errordict /estack` users: they see the
estack with the operator's slot already removed, not the slot
+ stop on top.

### 7.4 Snapshot cost

Each snapshot is one `TokenArray::push_back` per stack slot, plus
refcount bumps for pointer-payload tokens. For a 50-deep estack,
the snapshot is ~0.3 µs (B6 family benchmarks measure this).
Snapshots are off the hot path (errors are rare).

### 7.5 PostScript divergence

Red Book §3.10 prescribes a per-error-name handler dispatch via
`errordict`:

    errordict /<errorname> get exec

In standard PostScript, this is how `handleerror`,
`signalerror`, and user-defined error handlers compose. The
errordict can be augmented with custom handlers for new error
names.

**sli3 hardcodes the path**: raiseerror directly pushes `stop`
onto the e-stack, bypassing the `errordict /<errorname>` look-up.
This is a documented divergence (`implementation_spec.md`
Decisions log 2026-05-11). NEST 2.x worked the same way; user
code that relies on `errordict /MyError put` and then expects
`MyError raiseerror` to dispatch through that handler is not
supported.

Consequence: `signalerror` (gs's name for the dispatch-through-
errordict path) is not directly available. Users get a uniform
`stop`-based unwind regardless of error name.

---

## 8. Invariants the slice 8 dispatcher must preserve

This is the checklist that slice 8 implementation (or any other
dispatcher change) must satisfy. Each item is testable.

| # | Invariant | Test |
|---|---|---|
| I1 | Procedure body executes left-to-right; tokens are dispatched in proc-array order | `test_sli_eval` snippets exercising specific orderings |
| I2 | Nested procedure runs to completion before parent resumes | `test_sli_eval`: `{ { 1 2 add } exec 10 mul }` → 30 |
| I3 | `repeat` runs body exactly N times | `test_sli_eval`: `0 5 { 1 add } repeat` → 5 |
| I4 | `for` pushes counter to ostack before each body iteration | `test_sli_eval`: `0 1 1 3 { add } for` → 6 |
| I5 | `forall` pushes array element to ostack before each body iteration | `test_sli_eval`: `[10 20 30] {} forall` → ostack [10,20,30] |
| I6 | `exit` unwinds to the innermost loop's mark, no farther | `test_sli_eval`: `1 5 { { exit } loop add } for` (verify counter still works) |
| I7 | `stop` unwinds to the innermost stopped, ignoring marks | `test_sli_eval`: `1 5 { { stop } stopped pop add } for` |
| I8 | `stopped` pushes `true` on stop, `false` on normal completion | `test_sli_eval`: `{ stop } stopped` → true; `{ } stopped` → false |
| I9 | TCO: `{ proc } { proc } { proc } exec exec exec` does not grow the e-stack beyond a constant | new test `test_dispatch_topology.cpp::tco_chain` |
| I10 | After error, `$errordict /commandname` is the failing operator's name | `test_errors_dispatch` |
| I11 | After error with recordstacks=true, `$errordict /estack` array length matches the live estack at error time minus the failing op | `test_errors_dispatch` |
| I12 | Nested loop body-exhaust correctly resumes parent loop (no operand-stack corruption, no e-stack leak) | new test for cross-iter nesting |
| I13 | `cycles` count is non-decreasing and increments at least once per body token | parity test |
| I14 | OStack contents at the end of `{ body } exec` equal OStack contents at the end of inlined `body` | `test_dispatch_parity` |
| I15 | Operands pushed during last body iteration survive frame teardown | implicit in I4/I5 |
| I16 | The dictionary stack is NOT unwound by stop (known gap from PS) | documented, not tested as a positive feature |
| I17 | An iter-marker token (iiterate, irepeat, ifor, iforall) appearing inside a procedure body (constructed by hand) does not crash the dispatcher | new edge-case test (low priority; pathological input) |

---

## 9. PostScript conformance summary

| Feature | PS standard | sli3 | Notes |
|---|---|---|---|
| Procedure execution | §3.5 | ✓ | |
| Tail recursion | §3.5 | partial | iiterate single-level; multi-level only when slice 8 cascades |
| `loop`, `repeat`, `for`, `forall` | §3.11 | ✓ | |
| `exit` (innermost loop) | §3.11 | ✓ | error name `EStackUnderflow` instead of `invalidexit` |
| `stop` / `stopped` | §3.10 | ✓ | |
| stop with no stopped → defined error | §3.10 | partial | sli3 prints fatal + clears estack instead of named error |
| `errordict` per-name handler dispatch | §3.10 | ✗ | hardcoded `stop` path |
| Dictionary-stack restore on stop | §3.10 | ✗ | known gap |
| `signalerror` operator | §3.10 | ✗ | not exposed |
| `setglobal` / `currentglobal` (VM mode) | §3.8 | ✗ | not in scope (single VM) |
| `save` / `restore` (VM state) | §3.8 | ✗ | replaced by `savestate`/`restorestate` snapshot (different model) |
| `errordict` keys | §A.1 | partial | `newerror`, `errorname`, `commandname` per spec; `command` partially set |
| `recordstacks` flag | NEST 2.x extension | ✓ | not in PostScript |

The major divergences are all by design (single VM, no
per-error-name dispatch, no dict-stack unwind on stop). They're
captured here so a future PS-conformance push has a clear
starting point.

---

## 10. Open questions for review

1. **Should slice 8 implement multi-level body-exit cascade?** The
   spec says it MAY (§4.2 "SHOULD" but allows the
   single-cascade fallback). Strict reading of the Red Book does
   not require it — multi-level TCO is a quality-of-implementation
   issue.
2. **Should we close the dictionary-stack-restore gap?** Adding
   `save/restore`-like behaviour to stop is invasive and may
   surface latent bugs in code that mutates the dict stack
   transiently. Decide before slice 8 lands or defer to a later
   correctness slice.
3. **`exit` error name**: rename `EStackUnderflow` →
   `invalidexit` to match Red Book? Low cost, easy to break
   downstream user error handlers.
4. **Cycles count semantics**: I13 in §8 says "at least once per
   body token". Under slice 8's unified case, the inner
   `goto body_walk` may or may not cycle `++local_cycles`. Should
   we preserve today's count exactly (every token of every body
   counts), or relax to "cycles is monotone non-decreasing"?
5. **Iter-marker in proc body** (I17): currently undefined
   behaviour. The dispatcher's main switch would dispatch the
   marker as a frame-top, but the frame layout above wouldn't
   match. Should we raise an error? Today: silent corruption.

---

## 11. Test plan

Slice 8 lands with this spec's invariants as test gates. Each
invariant in §8 either has an existing test or needs one. New
tests go in:

- `tests/test_dispatch_parity.cpp`: I9, I12, I13, I14, I15
- `tests/test_errors_dispatch.cpp`: I10, I11
- `tests/test_dispatch_topology.cpp` (new): I9 (TCO chain),
  multi-level body exit, cross-iter-type cascade.

Each test runs through both `execute_` (plain mode) and
`execute_dispatch_` (dispatch mode) where applicable, asserting
identical OStack contents.

---

## 12. Glossary

- **Body**: the procedure body being walked by an iter case
  (`pick(2)` of any iter frame).
- **Continuation operator**: a function-style iter operator
  whose Token sits on the e-stack and is re-invoked between
  iterations (gs's pattern). `iloop`, `forall_s`,
  `forallindexed_*` are continuation operators in sli3.
- **Iter marker**: a typed-only Token whose tag identifies which
  iter type the frame is. `iiterate`, `irepeat`, `ifor`,
  `iforall` are iter markers. Dispatcher recognizes them and
  walks the body inline.
- **Body walk**: the inner loop of the dispatcher that fetches
  `proc[pos]`, advances pos, and dispatches the token.
- **Frame depth**: the number of e-stack slots an iter frame
  consumes (3 for iiterate, 4 for iloop, 5 for irepeat, 6 for
  iforall, 7 for ifor).
- **Mark**: a `marktype` Token at the bottom of an iter frame
  (for loop-style iters only). The unwind target of `exit`.
- **Stopped marker**: the literal name `::stopped` placed on the
  e-stack by StoppedFunction. The unwind target of `stop`.
- **TCO**: tail-call optimization. In sli3, single-level via
  iiterate's last-ref replacement; multi-level proposed for
  slice 8.

---

End of document.

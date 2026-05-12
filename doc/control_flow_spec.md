# sli3 control-flow execution contracts

Revision: 2026-05-12 (rev. 2 — review decisions applied). Status:
**accepted, awaiting implementation**. Anchor: commit `ee2eab7`
(draft landed), this revision pending commit.

**Review decisions (rev. 2):**

| # | Decision | Where applied |
|---|---|---|
| D1 | Multi-level body-exit cascade is **part of slice 8**. The cascade fires only at body-exhaust (rare) and is essential for fib-style recursion. | §3.3, §4.2, §8 (I9, I12) |
| D2 | Dict-stack restore on stop **follows PostScript**. `stopped` saves the dstack depth on entry; stop and normal completion both restore. | §6.2, §6.3, §9 |
| D3 | `exit` raises **`invalidexit`** when no enclosing loop exists. Renamed from sli3's previous `EStackUnderflow`. | §5.1, §5.2, §9 |
| D4 | `cycles` is **monotone non-decreasing and meaningful for debugging**. The exact relation to "number of body tokens dispatched" is implementation-defined; reproducibility across debug sessions matters more than a strict per-token count. | §8 (I13) |
| D5 | An iter-marker token appearing inside a procedure body **raises an error**. No silent corruption. | §3.2, §8 (I17) |

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
| iiterate/irepeat/ifor/iforall marker | **Raises `InvalidIterMarker` error** (D5). Iter markers are dispatcher-internal — they must only appear as the top of a freshly-pushed iter frame. Encountering one fetched out of a proc body means somebody constructed the proc by hand (e.g., from a saved snapshot whose layout shifted) and the frame layout invariants are broken. Raising preserves the operator-authoring contract from CLAUDE.md: "errors should never be silent". |

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

**Slice-8 scope (D1)**: iiterate implements single-level TCO via
the in-place last-ref replacement. irepeat/ifor/iforall do
**not** replace their proc slot (they need to preserve per-frame
counters across iterations). Multi-level TCO across nested
procs falls out for free from the body-exit cascade in §4.2:
when an inner iiterate frame body-exhausts, the cascade
immediately runs the parent's body-exhaust handler too if its
body is at end. Recursive code like `/fib { ... fib } def fib`
collapses an N-deep return chain into O(1) dispatcher cycles.

**PS conformance**: Red Book §3.5 ("Procedures … the language
guarantees that tail recursion does not consume execution-stack
space"). sli3 conforms for proceduretype-only chains. Loops do
not tail-recurse on body exhaustion (because the frame must
persist for the next iteration), but the body-exit cascade
makes nested loop teardown O(1) dispatcher cycles.

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

### 4.2 Multi-level body exit (D1)

When an inner frame finishes (body-exhaust pop) AND the new top
is another iter frame, the dispatcher **MUST** resume the outer
frame's body-walk directly — no outer-switch round trip. This is
the multi-level body-exit cascade.

**Today's behaviour** (pre slice 8): the inner case body
exhausts, `pop(N); break;`. The outer dispatch loop's switch
sees the outer iter marker on top, re-enters the appropriate
case, re-loads `proc`/`pos_p` from `pick(2)`/`pick(1)`, and
continues. One outer-switch round-trip per cascade step.

**Slice 8 behaviour**: the unified iter case checks the new top
after each body-exhaust; if it's an iter marker, re-load
`proc`/`pos_p` and `goto body_walk` directly. If the outer
body is also at end, its body-exhaust handler fires immediately
and may cascade further. The cascade unwinds an arbitrary
number of simultaneously-completed nested loop frames in a
tight inner loop, without entering the outer dispatcher switch.

**Worked example.** Consider:

```sli
1 1 3 {                         % outer for
   [ 10 20 30 ] {               %   middle forall
      2 { 42 pop } repeat       %     inner repeat
   } forall
} for
```

On the dispatcher cycle where all three loops simultaneously
hit "body done" (last repeat iter inside last forall element
inside last for counter):

```
[ irepeat | pos | proc | count=0 | mark
  iforall | pos | proc | idx=3   | array | mark
  ifor    | pos | proc | ctr=4   | lim=3 | inc=1 | mark
  ... ]
```

The cascade:

1. body-exhaust(irepeat): count=0 → `pop(5)`. New top is iforall.
2. body-exhaust(iforall): idx >= array.size() → `pop(6)`. New top is ifor.
3. body-exhaust(ifor): ctr > lim → `pop(7)`. New top not an iter marker → `break`.

One outer-switch round-trip instead of three.

**Invariants preserved.** The cascade is a fast path: the same
operands are pushed in the same order; the same e-stack
manipulations happen; only the timing changes. From any
operator's standpoint (including a user-supplied error handler
inspecting `$errordict /estack`), behaviour is identical to the
pre-cascade implementation.

**Cost.** One tag check per body-exhaust. Body-exhaust runs once
per loop iteration's body completion — at most O(loop count)
per cascade. For tight inner loops the cost is negligible
because body-exhaust is rare on the hot path.

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

Pseudocode (matches ExitFunction, D3 error rename pending):

    n = 1     # start at pick(1), skip exit's own slot at pick(0)
    while pick(n) not marktype and n < load:
        ++n
    if not found: raiseerror("invalidexit"); return   # D3: was "EStackUnderflow"
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

### 5.2 PostScript conformance (D3)

Red Book §3.11: "If there is no enclosing loop, exit issues an
`invalidexit` error." sli3 (post-D3) raises `invalidexit` — the
same name and same effect. Previously sli3 raised
`EStackUnderflow`; this is renamed to match the Red Book.

Implementation note: code under `src/builtins/sli_control.cpp`
(ExitFunction::execute) and the corresponding entry in
`src/interpreter/sli_interpreter.cpp` (`SLIInterpreter` member
initialisers — there is no current `invalidexit_name`; add one)
need updating. Any test or user-visible string referring to the
old name needs updating in lockstep. Audit:
`grep -rn EStackUnderflow src/ tests/ lib/sli/`.

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

### 6.2 sli3 implementation (D2 changes pending)

**StoppedFunction** pushes a frame that captures the **dstack
depth** at entry (D2 — for dict-stack restore on stop):

    Before:  EStack [ ... ]
             OStack [ proc | ... ]
             DStack depth = D₀
    After:   EStack [ proc | ::stopped | saved_dstack=D₀ | ... ]
             OStack [ ... ]
             DStack depth = D₀ (unchanged)

The frame is three slots:

| pick from top of frame | type | contents |
|---|---|---|
| 0 | proceduretype | the body being protected (dispatched first) |
| 1 | nametype | the literal `::stopped` marker |
| 2 | integertype | the dstack depth at the moment `stopped` was called |

The proc is pushed on top so it dispatches first. After it
completes normally, ::stopped is on top; it resolves via dict
lookup and runs cleanup (D2 — see below). On stop, stop unwinds
to ::stopped and restores the dstack depth before pushing
`true`.

**Cleanup contract for ::stopped.** Under D2, `::stopped` is no
longer a plain name bound to `false`. It is a continuation
operator (a functiontype Token) whose body:

1. Pops the `saved_dstack` slot from the e-stack (one slot
   below itself).
2. Pops dicts from the dictionary stack until DStack.load() ==
   saved_dstack.
3. Pushes `false` onto the operand stack.

This gives normal-completion semantics matching the Red Book:
"the dictionary stack is restored to its state at the entry to
stopped".

**StopFunction** walks the e-stack looking for the literal name
`::stopped` (the marker, identified by its nametype + name
value). Pseudocode:

    n = 1
    while pick(n) != ::stopped and n < load:
        ++n
    if !found:
        message(M_FATAL, "stop", "No stopped context");
        EStack.clear()
        return
    # Restore dstack depth (D2):
    saved = pick(n + 1).data_.long_val
    while DStack.load() > saved: DStack.pop()
    # Now unwind the e-stack:
    OStack.push(true)
    EStack.pop(n + 2)   # includes stop + frames up through ::stopped + saved_dstack

Note: stop's pop INCLUDES the ::stopped marker AND the
saved_dstack slot. Operand stack gets `true` pushed after the
unwind.

**Why a separate saved_dstack slot rather than encoding it in
::stopped?** Two reasons. (1) Keeps the `::stopped` marker
recognizable as a constant name in stop's walk — no per-call
data to inspect. (2) Lets normal completion handle the cleanup
in a single continuation-op invocation without special-casing
the dispatcher.

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

**Invariant (D2)**: the dictionary stack **is** unwound by stop
to the depth saved at `stopped` entry. PS standard §3.10
requires this: "the dictionary stack is automatically restored
to the state it had at the `stopped` call". Implementation
detail in §6.2; before D2 lands this is a known gap and stop
leaves the dstack untouched. After D2 lands, stop pops dicts
from the dstack until depth matches the saved value.

Implication for SLI scripts that do `dict begin ... stop`: any
`begin` that runs inside the stopped context but doesn't have a
matching `end` before stop fires will be auto-undone. This is a
**behaviour change** for any user code that relied on
leaked-dict semantics (NEST 2.x did not implement this either,
so leakage was technically possible but discouraged).

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
| I13 | **(D4 relaxed)** `cycles` is monotone non-decreasing across a script's execution, and increments enough that two consecutive significant dispatcher events have different cycle values (i.e. cycles is useful for "did the interpreter make progress" debugging). The exact relation to body-token count is implementation-defined. | parity test (verify monotonicity, not exact count) |
| I14 | OStack contents at the end of `{ body } exec` equal OStack contents at the end of inlined `body` | `test_dispatch_parity` |
| I15 | Operands pushed during last body iteration survive frame teardown | implicit in I4/I5 |
| I16 | **(D2 promoted)** The dictionary stack IS unwound by stop to the depth saved at `stopped` entry. Same restore happens on normal completion via `::stopped`'s cleanup. | new test `test_dispatch_topology.cpp::stop_restores_dstack` |
| I17 | **(D5 strengthened)** An iter-marker token (iiterate, irepeat, ifor, iforall) appearing inside a procedure body (constructed by hand or loaded from a saved snapshot) **raises an `InvalidIterMarker` error**. No silent corruption, no crash. | new edge-case test `test_dispatch_topology.cpp::iter_marker_in_body_raises` |

---

## 9. PostScript conformance summary

| Feature | PS standard | sli3 (post-slice-8) | Notes |
|---|---|---|---|
| Procedure execution | §3.5 | ✓ | |
| Tail recursion | §3.5 | ✓ (D1) | iiterate single-level + multi-level via cascade |
| `loop`, `repeat`, `for`, `forall` | §3.11 | ✓ | |
| `exit` (innermost loop) | §3.11 | ✓ (D3) | error name now `invalidexit` per Red Book |
| `stop` / `stopped` | §3.10 | ✓ (D2) | |
| stop with no stopped → defined error | §3.10 | partial | sli3 prints fatal + clears estack; the print is informative, not an actual named error |
| `errordict` per-name handler dispatch | §3.10 | ✗ | hardcoded `stop` path — by design |
| Dictionary-stack restore on stop | §3.10 | ✓ (D2) | implemented via saved-dstack slot in `stopped` frame |
| `signalerror` operator | §3.10 | ✗ | not exposed — by design |
| `setglobal` / `currentglobal` (VM mode) | §3.8 | ✗ | not in scope (single VM) |
| `save` / `restore` (VM state) | §3.8 | ✗ | replaced by `savestate`/`restorestate` snapshot (different model) |
| `errordict` keys | §A.1 | partial | `newerror`, `errorname`, `commandname` per spec; `command` partially set |
| `recordstacks` flag | NEST 2.x extension | ✓ | not in PostScript |

The remaining divergences are all by design (single VM, no
per-error-name dispatch). They're captured here so a future
PS-conformance push has a clear starting point. The fatal-on-
no-stopped behaviour is a minor cosmetic divergence — gs does
the same thing differently (prints, returns to REPL).

---

## 10. Resolved review decisions

All five open questions from the rev-1 draft have been
resolved. They are listed here for traceability.

1. **Multi-level body-exit cascade** → **D1, in scope of slice 8.**
   Body-exhaust pop checks the new top; if it's an iter marker,
   re-enter the body-walk for that frame. Recursive fib-style
   code collapses an N-deep return chain into O(1) dispatcher
   cycles. See §4.2.
2. **Dictionary-stack restore on stop** → **D2, follow PS
   standard.** `stopped` records DStack depth in a third frame
   slot; both stop and normal completion restore DStack to that
   depth before pushing the bool result. See §6.2.
3. **`exit` error name** → **D3, rename to `invalidexit`.**
   Matches Red Book §3.11. Audit + update at slice-8 landing
   time. See §5.1, §5.2.
4. **Cycles count semantics** → **D4, relax to monotone
   non-decreasing with debugging utility.** Exact per-token count
   is not part of the contract; reproducibility of "did the
   interpreter make progress" between debug sessions is. See I13
   in §8.
5. **Iter-marker in proc body** → **D5, raise an error.** The
   token type `iiterate/irepeat/ifor/iforall` appearing as a
   fetched proc token raises `InvalidIterMarker`. No silent
   corruption. See §3.2, I17 in §8.

(Reserved for future decisions: this section accumulates new
open questions as the spec evolves.)

---

## 11. Test plan

Slice 8 lands with this spec's invariants as test gates. Each
invariant in §8 either has an existing test or needs one. New
tests go in:

- `tests/test_dispatch_parity.cpp`: I9, I12, I13 (monotone
  cycles), I14, I15.
- `tests/test_errors_dispatch.cpp`: I10, I11; also update for D3
  (rename `EStackUnderflow` → `invalidexit` assertions where
  they exist).
- `tests/test_dispatch_topology.cpp` (new): I9 (TCO chain),
  multi-level body exit cascade (D1), cross-iter-type resume,
  D2 (`stop_restores_dstack`, `normal_completion_restores_dstack`),
  D5 (`iter_marker_in_body_raises`).

Each test runs through both `execute_` (plain mode) and
`execute_dispatch_` (dispatch mode) where applicable, asserting
identical OStack contents.

**D2 test sketches.**

```cpp
// stop_restores_dstack
//
// Before: DStack.load() = D0
// Run:    { <<>> begin <<>> begin stop } stopped
// After:  DStack.load() == D0  (the two begins were undone by stop)
//         OStack top == true   (stopped caught the stop)

// normal_completion_restores_dstack
//
// Before: DStack.load() = D0
// Run:    { <<>> begin <<>> begin } stopped
// After:  DStack.load() == D0  (begins undone by ::stopped's cleanup)
//         OStack top == false  (normal completion)
```

**D5 test sketch.**

```cpp
// iter_marker_in_body_raises
//
// Construct a proceduretype Token whose array contains an
// iiteratetype Token directly (cannot be done from SLI source;
// build via TokenArray API in C++).
// Run: <constructed proc> exec
// After: raiseerror("InvalidIterMarker") fires; $errordict /errorname
//        == /InvalidIterMarker; OStack reflects the error path.
```

**D3 audit.** `grep -rn "EStackUnderflow" src/ tests/ lib/sli/`:
the ExitFunction raise site is the only production one; the
test_errors family asserts on the name string. Rename to
`invalidexit` everywhere.

**D4 cycles test.** Existing tests should NOT assert on exact
cycle counts. Audit: `grep -rn "cycles\b" tests/`. Add a test
that asserts cycles is strictly increasing between two
checkpoints separated by at least one dispatcher cycle.

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

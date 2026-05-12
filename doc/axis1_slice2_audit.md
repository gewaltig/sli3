# Axis I Slice 2 — nested-op attribution audit

Date: 2026-05-12.

Slice 2's job is to inventory the direct `fn.execute(i)` call sites
in `src/builtins/` (Stage 9 dispatchers calling typed leaves), and
decide, per site, whether the inner op or the outer op should be
named in `$errordict /commandname` when an error fires inside the
inner.

Slice 1 was deferred to Slice 4, so `current_op_` does not exist
yet. The audit's output is the *specification* for Slice 4 — what
each call site needs to look like once `current_op_` is in place.

## The two cases

After Slice 4 lands `current_op_`, the dispatcher's functiontype arm
writes `current_op_ = fn; ... ; current_op_ = nullptr;` around each
operator call. When operator A is dispatched, `current_op_ = &A`.

Now A's `execute()` body invokes operator B directly via
`B_fn.execute(i)`. What happens to `current_op_`?

- **Default (no wrap)**: `current_op_` stays at `&A` during B's
  execution. If B raises, `$errordict /commandname` reports `/A`.
  → **outer attribution**.
- **Explicit wrap**: A's body does
  `auto saved = i->current_op_; i->current_op_ = &B;
   B_fn.execute(i); i->current_op_ = saved;`. If B raises,
  `$errordict /commandname` reports `/B`. → **inner attribution**.

The plan's text mentions inner attribution as the gs-style default,
but in our codebase that's only true for ops *dispatched through
the dispatcher*. A direct `.execute(i)` call from one operator's C
body to another is **not** dispatched through the dispatcher —
it's a synchronous C++ call. The dispatcher only set `current_op_`
once, when it entered the outer op. So our actual default for
direct calls is **outer attribution**.

That's what we want, almost everywhere.

## The pattern in this codebase

Every direct `.execute(i)` site in `src/builtins/` is a **Stage 9
compaction dispatcher** calling its typed leaf. The compaction
work moved each common operator (`get`, `put`, `length`, `forall`,
`def`, `cvi`, `cvd`, `cvx`, ...) from a trie-bound name to a
single C++ function that switches on operand tags and invokes the
appropriate typed leaf (`get_a`, `put_a`, `length_s`, ...) by
direct call. The user-facing name is always the dispatcher; the
typed leaf is an implementation detail.

So if a user writes:

```sli
[1 2 3] 99 get
```

the dispatcher sees `/get`, sets `current_op_ = &GetFunction`,
calls `GetFunction::execute(i)`. The body inspects the operand
tags, decides "array + int", and calls `get_a_fn.execute(i)`. If
`get_a_fn` raises `RangeCheck`, the user sees `$errordict
/commandname` = `/get` — which is what they typed, and what they
expect to see.

→ Outer attribution is correct, default behavior is correct,
**no wrap needed**.

If a user writes `[1 2 3] 99 get_a` directly (rare; only addtotrie
users), the dispatcher sees `/get_a` directly, sets
`current_op_ = &get_a_fn`, and `current_op_` is `/get_a` when the
range error fires. Also correct.

## Inventory

Source: `grep -rn '\.execute(i)' src/builtins/` (2026-05-12,
51 sites). All sites follow the same pattern: a Stage 9 dispatcher
function calling one of its typed leaves on the same C++ stack
frame. Grouped by file:

### `src/builtins/sli_control.cpp` (5 sites)

| Line | Calling op (dispatcher) | Called op (typed leaf) | Decision |
|------|-------------------------|------------------------|----------|
| 876  | `ForallFunction` (`/forall`) | `forall_afunction` (`/forall_a`) | outer (default) |
| 879  | `ForallFunction` | `forall_sfunction` (`/forall_s`) | outer (default) |
| 915  | `ForallindexedFunction` (`/forallindexed`) | `forallindexed_afunction` (`/forallindexed_a`) | outer (default) |
| 918  | `ForallindexedFunction` | `forallindexed_sfunction` (`/forallindexed_s`) | outer (default) |
| 960  | `DefDispatchFunction` (`/def`) | `deffunction` (`/def_`) | outer (default) |

### `src/builtins/sli_typecheck.cpp` (6 sites)

| Line | Calling op (dispatcher) | Called op (typed leaf) | Decision |
|------|-------------------------|------------------------|----------|
| 423  | `CvxFunction` (`/cvx`) | `cvx_a_fn` (`/cvx_a`) | outer (default) |
| 465  | `CviFunction` (`/cvi`) | `integerfunction` (`/int_d`) | outer (default) |
| 489  | `CvdFunction` (`/cvd`) | `doublefunction` (`/double_i`) | outer (default) |
| 514  | `CvlitFunction` (`/cvlit`) | `cvlit_nfunction` (`/cvlit_n`) | outer (default) |
| 517  | `CvlitFunction` | `cvlit_pfunction` (`/cvlit_p`) | outer (default) |
| 544  | `CvnFunction` (`/cvn`) | `cvn_lfunction` (`/cvn_l`) | outer (default) |

### `src/builtins/sli_container_ops.cpp` (40 sites)

All sites are Stage 9 dispatchers (`/join`, `/getinterval`,
`/length`, `/get`, `/put`, `/empty`, `/size`, `/reserve`,
`/insertelement`, `/search`, `/cva`, `/append`, `/prepend`)
calling their typed leaves. The list:

| Line range | Calling op | Called ops |
|------------|------------|------------|
| 1277-1279  | `JoinFunction` (`/join`) | `join_a_fn`, `join_s_fn`, `join_p_fn` |
| 1287-1288  | `GetintervalFunction` (`/getinterval`) | `getinterval_a_fn`, `getinterval_s_fn` |
| 1297-1301  | `LengthFunction` (`/length`) | `length_a_fn`, `length_p_fn`, `length_lp_fn`, `length_s_fn`, `length_d_fn` |
| 1321-1334  | `GetFunction` (`/get`) | `get_a_fn`, `get_a_a_fn`, `get_p_fn`, `get_lp_fn`, `get_s_fn`, `get_d_fn` |
| 1354-1367  | `PutFunction` (`/put`) | `put_a_fn`, `put_a_a_t_fn`, `put_p_fn`, `put_lp_fn`, `put_s_fn`, `put_d_fn` |
| 1385-1387  | `EmptyFunction` (`/empty`) | `empty_a_fn`, `empty_d_fn`, `empty_s_fn` |
| 1397-1398  | `SizeFunction` (`/size`) | `size_a_fn`, `size_s_fn` |
| 1408-1409  | `ReserveFunction` (`/reserve`) | `reserve_a_fn`, `reserve_s_fn` |
| 1419-1420  | `InsertelementFunction` (`/insertelement`) | `insertelement_a_fn`, `insertelement_s_fn` |
| 1435-1436  | `SearchFunction` (`/search`) | `search_a_fn`, `search_s_fn` |
| 1452       | `CvaFunction` (`/cva`) | `cva_d_fn` |
| 1635-1637  | `AppendFunction` (`/append`) | `append_a_fn`, `append_p_fn`, `append_s_fn` |
| 1646-1648  | `PrependFunction` (`/prepend`) | `prepend_a_fn`, `prepend_p_fn`, `prepend_s_fn` |

**All 40 sites: outer (default).**

### Other patterns inspected — no audit hits

- `baselookup(Name)` followed by `EStack().push(...)`: the looked-up
  op is *scheduled* on the e-stack and dispatched normally by the
  dispatcher's main loop after the caller returns. Not a nested
  call. `current_op_` is set by the dispatcher when the inner op
  fires, inner attribution applies. Examples:
  - `sli_control.cpp:121` (`IloopFunction` pushes `/::loop`)
  - `sli_control.cpp:585` (`OpenofstreamFunction` pushes `/end`
    to close after use)
  - `sli_control.cpp:721, 786, 814, 842` (forall variants push
    the iforall_* continuation)
  - `sli_control.cpp:889` (`ForallFunction` pushes `/forall_di`
    for dict input — the SLI proc machinery handles it)
  - `sli_control.cpp:953` (`DefDispatchFunction` pushes
    `/:def_` for the 3-arg trie-checked form)
  - `sli_control.cpp:1196, 1236` (`StopFunction` /
    `StoppedFunction` push `/::pop`)
  - `sli_array_module.cpp:723, 744, 807` (Map / MapIndexed /
    MapThread schedule their iterator)
  - `sli_container_ops.cpp:1459` (`CvaFunction` for tries
    schedules `/cva_t`)
  - `sli_typecheck.cpp:427, 437, 470, 495, 525, 551` (cv* arms
    that route to SLI-defined helpers via baselookup)
  Each correctly reports the inner op (via the dispatcher's
  current_op_ write at the time of inner dispatch).

- `setup_map_frame_` (`sli_array_module.cpp:568`): helper that
  prepares an iterator frame on the e-stack. Pushes the iterator
  Token; does not call `.execute(i)`. Same as baselookup-push:
  inner attribution via the dispatcher.

## Summary

| Site count | Attribution | Code change at Slice 4 |
|-----------:|-------------|------------------------|
| 51 / 51    | outer (default) | none (no wrap) |
| 0          | inner (explicit wrap) | n/a |

**Slice 2 conclusion: zero code changes.** When Slice 4 lands
`current_op_`, the dispatcher will set it before each op call and
clear after; the 51 direct `.execute(i)` sites in `src/builtins/`
need no wrapping because they all want the default outer-attribution
that the unmodified field provides.

## Test plan for Slice 4

`tests/test_errors_dispatch.cpp` should grow at least one case per
dispatcher in the inventory: trigger the typed leaf's error path
and assert `$errordict /commandname == /<dispatcher-name>`.
Today's behavior (no `current_op_`, e-stack-top read) attributes
correctly for dispatcher → leaf calls because the e-stack top is
the dispatcher's function Token when the leaf is on the C++ call
stack. After Slice 4, the same case must produce the same result
via `current_op_` instead.

Minimal regression coverage that protects this audit:

```cpp
// 1. arr 99 get -> RangeCheck on /get
"[1 2 3] 99 get stopped {errordict /commandname get} if"
// expected ostack top: /get  (NOT /get_a)

// 2. {} {} forall -> ArgumentType on /forall (or similar)
"{ } { } forall stopped {errordict /commandname get} if"
// expected: /forall  (NOT /forall_a)

// 3. /x def_ with wrong arity -> StackUnderflow on /def
"/x def stopped {errordict /commandname get} if"
// expected: /def  (NOT /def_)
```

These cases prove outer attribution is preserved through Slice 4's
ABI change. Add when Slice 4 lands; not a blocker for Slice 2's
audit.

## Pointers for Slice 4

When Slice 4 writes the dispatcher's `current_op_ = fn` wrap, it
does **not** need to touch any of the 51 sites listed here. The
default current_op_ behavior produces correct attribution for all
of them.

Slice 4 *does* need to leave the `current_op_` field unchanged
across the body of these sites — i.e. the dispatcher's wrap
should be set/clear (not save/restore) so the inner C call doesn't
overwrite a stale state. (The dispatcher already does set/clear
in the revert-Slice-1 design.)

## What would *change* this audit

If a future operator wants inner-op attribution at a direct
`.execute(i)` site (rare; an example would be a wrapper that
deliberately exposes the inner op's name as the user-facing
identity), it would need to explicitly wrap:

```cpp
auto saved = i->current_op_;
i->current_op_ = &inner_fn;
inner_fn.execute(i);
i->current_op_ = saved;
```

No such operator exists today. If one is added later, that
site is added to this audit with `Decision = inner` and the
wrap pattern is applied.

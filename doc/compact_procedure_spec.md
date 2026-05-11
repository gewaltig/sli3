# Compact procedure storage — implementation spec

Self-contained spec for the next stage of dispatcher optimization.
Hand to a fresh Claude session along with `CLAUDE.md`, `fix-plan.md`,
and `doc/hot_ops.md`.

## Background

Stage 9 (tag `stage9-complete`) removed the trie indirection from
every common operator. Bench standing today (`bench/run.sh`,
`doc/hot_ops.md`):

| Bench | sli3 | gs 10.07 | gap |
|-------|------|----------|-----|
| B1  `1 pop` × 100M | 1.33 s | 1.32 s | tied |
| B2  `1 1 add pop` × 100M | 2.40 s | 2.70 s | sli3 −11 % |
| **B2b** `{1 1 add pop} bind repeat` × 100M | **1.91 s** | **1.23 s** | **+55 %** |
| B3  100k × 1k for | 2.10 s | 2.58 s | sli3 −19 % |
| B5  dict + dictstack × 100M | 16.34 s | 24.67 s | sli3 −33 % |

The one workload where gs still wins decisively is **B2b** —
a pre-bound procedure body executed 100M times. sli3 spends
~19 ns/iter there; gs spends ~12 ns. The gap is **per-token
iteration overhead**, not name lookup (`bind` already removed
that), not the operator body, not allocation. It is the cost
of walking a `TokenArray` of 16-byte Tokens and dispatching
each one through the outer switch + (for operators) a virtual
call into `SLIFunction::execute`.

## Goal

Close the B2b gap to within 15 % of gs (≤ 1.41 s) without
breaking any of the existing wins. Realistic target: 8–12 ns
per token on `{1 1 add pop} bind repeat`.

The mechanism is the GhostScript idea adapted to our types:
after `bind`, swap the procedure body's storage from a
`std::vector<Token>` (16 B/elt) to a packed array of 4-byte
slots, plus per-procedure spill pools for values that don't
fit inline. The dispatcher learns a parallel iteration path
for compact bodies.

## What we keep — PostScript semantics

Late binding is the default in PostScript: a procedure body
is a literal sequence of tokens, and each name in it is
re-resolved every time the procedure runs.

```sli
/add 1 def
10 {add} repeat    % each iter: lookup `add` -> integer 1; push it
10 {=}   repeat    % prints ten 1s
```

This must continue to work unchanged. The compactor never
collapses a `nametype` token into anything that bypasses the
dictstack — **unless `bind` was called**, which is the user's
explicit opt-in to freeze name → value at that moment.

## The compact slot format

```cpp
enum CompactTag : uint8_t {
    CT_INT_SMALL = 0,   // 24-bit signed int          (range ~-8M..+8M)
    CT_BOOL,            // payload bit
    CT_OPERATOR,        // payload = HotOp id          (after bind only)
    CT_FUNCTION,        // payload = index into fns[]  (after bind only)
    CT_NAME,            // payload = Name handle       (late-bind path)
    CT_LITERAL,         // payload = Name handle       (executable=false literal)
    CT_DBL_INDEX,       // payload = index into doubles[]
    CT_NESTED_INDEX,    // payload = index into spilled[] (Token storage for
                        //           sub-procs, strings, dicts, big ints, ...)
};

struct CompactSlot {     // 4 bytes
    CompactTag tag;      // 1 byte
    uint8_t    aux;      // 1 byte (small-int high byte; reserved for the
                         //         others -- pads to 4)
    uint16_t   payload;  // 2 bytes (24-bit ints use aux+payload together)
};

class CompactProc {
public:
    std::vector<CompactSlot>   slots;
    std::vector<double>        doubles;   // CT_DBL_INDEX targets
    std::vector<SLIFunction*>  fns;       // CT_FUNCTION targets (not HotOp)
    std::vector<Token>         spilled;   // CT_NESTED_INDEX targets
};
```

Layout choice: 4 B/slot is the sweet spot.
- 2 B (gs's choice) is tighter but forces a side table for
  every non-trivial value including small ints > 2^14, every
  Name (24-bit handles), every operator (we have ~50 hot ops
  + a many SLIFunctions). The pool indirection costs more than
  the cache footprint saves.
- 4 B keeps the body of `{1 1 add pop}` at 16 bytes (1/4 of
  the current 64) while keeping nearly everything inline.
- 8 B duplicates work without gain — we'd still need a side
  pool for doubles and strings.

`TokenArray` stays unchanged. `CompactProc` is a separate type.
A Token of `proceduretype` whose `data_.array_val` points to a
`TokenArray` is the today-form; a new flag on the array (or a
new typeid `compactproceduretype` — append-only, bumps
`kSerializeVersion`) indicates the body is a `CompactProc`
instead.

## The compactor

Lives in C++, replaces the SLI-defined `bind` in
`lib/sli/sli-init.sli` lines 284–309. Public name: `bind`. Body:

```cpp
class BindFunction : public SLIFunction {
public:
    void execute(SLIInterpreter* i) const override {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::proceduretype);
        TokenArray* src = i->top().data_.array_val;
        CompactProc* dst = build_compact(src, i);
        i->top() = make_compact_proc_token(dst, i);
        i->EStack().pop();
    }
};
```

`build_compact(TokenArray*, SLIInterpreter*)` walks the body
once and emits a slot per token. Per-token disposition:

- `integertype`, value fits 24-bit signed → `CT_INT_SMALL`.
- `integertype`, doesn't fit → spill, `CT_NESTED_INDEX`.
- `doubletype` → push into `doubles`, `CT_DBL_INDEX`.
- `booltype` → `CT_BOOL`.
- `literaltype` → `CT_LITERAL` (carry the Name handle).
- `nametype` → lookup at this moment via dictstack. If it
  resolves to a `functiontype` whose `hot_op() != Generic`,
  emit `CT_OPERATOR`. If it resolves to a `functiontype`
  with no HotOp, store the `SLIFunction*` in `fns` and emit
  `CT_FUNCTION`. Otherwise (no resolution, or resolves to
  something non-callable like a number — that's
  `/add 1 def`'s case) leave it as `CT_NAME(handle)` for
  late binding.
- `proceduretype` (nested proc) → recursively bind the
  nested proc into another `CompactProc`, push that as a
  Token into `spilled`, emit `CT_NESTED_INDEX`. This is what
  PostScript's `bind` does recursively.
- `stringtype`, `dictionarytype`, `arraytype` literal,
  anything else → spill the Token, emit `CT_NESTED_INDEX`.

Bind is idempotent: a `proceduretype` Token whose body is
already a `CompactProc` returns unchanged.

## Dispatcher integration

The four iter cases in `sli_interpreter.cpp` (`iiteratetype`,
`irepeattype`, `ifortype`, `iforalltype`) each gain a fast
path keyed on `is_compact(proc)`:

```cpp
case sli3::irepeattype: {
    if (proc->is_compact()) {
        CompactProc* cp = proc->compact();
        long& pos = execution_stack_.pick(1).data_.long_val;
      start_compact_repeat:
        if (pos < static_cast<long>(cp->slots.size())) {
            CompactSlot s = cp->slots[pos++];
            switch (s.tag) {
              case CT_INT_SMALL: {
                long v = decode_int24(s);
                operand_stack_.push(new_token<sli3::integertype>(v));
                goto start_compact_repeat;
              }
              case CT_OPERATOR:
                ops::dispatch_hot_inline(static_cast<HotOp>(s.payload), this);
                goto start_compact_repeat;
              case CT_FUNCTION:
                cp->fns[s.payload]->execute(this);
                if (execution_stack_.top().tag() == sli3::irepeattype)
                    goto start_compact_repeat;
                break;
              case CT_NAME: {
                // late-binding path -- preserves /add 1 def semantics
                const Token& resolved = lookup(s.payload);
                if (resolved.tag() == sli3::functiontype) {
                    SLIFunction* fn = resolved.data_.func_val;
                    if (!ops::dispatch_hot(fn->hot_op(), this))
                        fn->execute(this);
                    if (execution_stack_.top().tag() == sli3::irepeattype)
                        goto start_compact_repeat;
                    break;
                }
                if (resolved.is_executable()) {
                    execution_stack_.push(resolved);
                    break;
                }
                operand_stack_.push(resolved);
                goto start_compact_repeat;
              }
              case CT_NESTED_INDEX:
                execution_stack_.push(cp->spilled[s.payload]);
                break;
              // ... CT_DBL_INDEX, CT_LITERAL, CT_BOOL
            }
            break;  // case CT_FUNCTION / CT_NAME may need outer dispatch
        }
        // loop tail: same as the existing unpacked path
        ...
    }
    // fall through to existing TokenArray path
    ...
}
```

The `goto`-driven inner loop matches the existing pattern in
`execute_dispatch_`. The `CT_OPERATOR` arm is the win: one
shifted load, one switch arm, direct call to the inline op
body in `sli_op_bodies.h`. No virtual dispatch, no estack
push for the function token.

## Test plan

Add `tests/test_compact_proc.cpp`:

1. **Parity tests.** For each B-bench script, evaluate it
   twice: once with bind, once without. Compare ostack at the
   end (B-benches push timing values; reduce to a hash for
   comparison). Required for every operator already covered
   by `test_sli_eval.cpp`.

2. **Late binding preserved.** `/add 1 def 10 {add} repeat`
   must push ten 1s, both before and after `bind`-ing the
   inner procedure separately. The compactor's `CT_NAME`
   arm carries the test weight here.

3. **Bind freezes operators.** `/p {1 add} bind def /add 1
   def 5 p` must return 6, not be affected by the redef.

4. **Spillover correctness.** Doubles, big ints, strings,
   nested procs all round-trip through compaction and run
   correctly.

5. **`cvx` array→proc** produces an unbound proc (`TokenArray`,
   not `CompactProc`). The user has to `bind` explicitly to
   compact it.

6. **Mutation.** `/p {1 add} def p 0 5 put` — putting into a
   compacted body. Simplest correct semantics: a `put` on a
   compacted proc decompacts it back to TokenArray first.
   `tests/` must cover this.

7. **Introspection.** `length`, `==` printing, `cva` on a
   compacted proc all return the same result as on the
   unpacked version.

8. **ASAN/UBSan** clean.

## Bench expectations

Working hypothesis, to be confirmed:

| Bench | today | predicted | reasoning |
|-------|------:|----------:|-----------|
| B1 (no bind) | 1.33 s | 1.30 s | small cache win on 2-token body |
| B2 (no bind) | 2.40 s | 2.30 s | same; nametype arm dominates |
| **B2b (bound)** | **1.91 s** | **~1.3 s** | direct op dispatch; cache win |
| B3 (bound inner) | 2.10 s | ~1.9 s | for-loop body benefits |
| B5 (no bind) | 16.34 s | 16.0 s | small cache win, lookup dominates |

If B2b lands ≤ 1.4 s (within 15 % of gs's 1.23 s) the spec
is delivered. If it lands worse than 1.7 s, re-investigate.

## Implementation order (one slice per commit)

1. `CompactProc` type + builder unit-tested in isolation.
2. C++ `BindFunction`, registered as `/bind` (replaces the
   SLI-defined `bind` in `sli-init.sli`).
3. Dispatcher path for `irepeattype` (matches B2b).
   Measure. If B2b within range, proceed.
4. Same for `iiteratetype`, `ifortype`, `iforalltype`.
5. Introspection ops: `length_p`, `length_lp`, `get_p`,
   `put_p`, `cva` on procs, debug printing.
6. Mutation: decompact-on-`put`.
7. Serialization: a `compactproceduretype` typeid that
   serializes to plain proceduretype (i.e. write the
   unpacked body; reload reconstructs a TokenArray; user
   re-binds if they want it compact again). Append to
   `sli_typeid` and bump `kSerializeVersion`.
8. Update `doc/hot_ops.md` bench table.

Each slice is independently testable and commitable. Don't
combine slice 3 with the others before its bench result is
in.

## Risks

- **Mutation correctness**: easy to get wrong if any code
  path silently mutates a compacted proc. Audit every
  `data_.array_val` write site on `proceduretype` Tokens.
  Tests #6 above is the safety net.
- **Bind recursion**: nested procs in an array literal that's
  also a procedure body — `{[{x}] y}`. The compactor must
  recurse into procedure bodies but not into array literals
  *unless they hold procs that bind would recurse into*. NEST
  2.x's bind walks arrays-of-procs; check the upstream
  reference (`/Users/gewaltig/Code/nest-2.20.2/sli/`) for the
  rule we want to match.
- **Memory pressure**: compact bodies add allocations
  (`std::vector<CompactSlot>` + side pools). For a procedure
  used once, that's overhead. Keep `bind` opt-in; never
  compact implicitly.
- **Debugger output**: `==` on a compact proc must round-trip
  through the parser. Easiest: print the procedure by walking
  slots and emitting each as if it were a Token (operator
  arms expand to the operator's name via the systemdict
  binding).
- **Pointer-tagging interaction**: SLIType* tagging in
  `Token::type_` is unaffected — the compact form has no
  Tokens-in-the-body, so the question doesn't arise.

## File pointers (current sli3 layout)

| What | Path |
|------|------|
| Token / SLIType layout | `src/types/sli_token.h`, `src/types/sli_type.h` |
| TokenArray | `src/containers/sli_array.h` |
| SLIFunction base + HotOp enum | `src/interpreter/sli_function.h` |
| Hot-op inline bodies | `src/builtins/sli_op_bodies.h` |
| Dispatcher | `src/interpreter/sli_interpreter.cpp` (look for `execute_dispatch_` and the four iter cases) |
| Existing `bind` (SLI) | `lib/sli/sli-init.sli` lines 284–309 |
| Existing nametype/functiontype inline | `sli_interpreter.cpp` in iter cases (search for `// Inline name -> function dispatch`) |
| Bench harness | `bench/run.sh`, `bench/sli/B*.sli`, `bench/ps/B*.ps` |
| Catalog | `doc/hot_ops.md` |

## Out of scope

- 2-byte slot encoding (gs's `ref_packed`). 4 B is good
  enough; revisit only if profiling justifies the extra
  side-table cost.
- Computed-goto threaded dispatch inside the iter cases. The
  super-instruction experiment from Stage 9 showed virtual
  calls are well-predicted on M2; threaded code is unlikely
  to help further until we have measurements suggesting the
  switch itself is the bottleneck.
- Compaction of arrays that are not executable. PostScript
  bind doesn't touch literal arrays; we follow.

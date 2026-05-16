# Adding a new C++ operator

Revision: 2026-05-16. Status: **working notes** — captures the
patterns the codebase already uses. Read alongside `CLAUDE.md`
(architectural facts) and `doc/control_flow_spec.md` (dispatcher
contract). When the two disagree, the source wins; please fix the
doc and link the commit.

This guide walks through the actual decisions you make when adding
an operator: which file, which class shape, which dispatch
pattern, how it gets registered. The end-to-end example at the
bottom (`Last`) ties everything together.

---

## 1. Picking a file

Operators are grouped by topic under `src/builtins/`:

| File | What lives there |
|---|---|
| `sli_stack.cpp` | Pure-stack ops: `pop`, `dup`, `exch`, `roll`, `index`, `mark`, `counttomark`. |
| `sli_math.cpp` | Arithmetic and comparisons: `add`, `sub`, `mul`, `eq`, `lt`, integer/double leaves. |
| `sli_control.cpp` | Control flow + procedure machinery: `if`, `ifelse`, `for`, `repeat`, `forall`, `def`, `bind`, error ops. |
| `sli_typecheck.cpp` | Type predicates and conversions: `type`, `cvi`, `cvd`, `cvs`, `cvlit`, the `trie` family. |
| `sli_array_module.cpp` | Array stdlib: `Range`, `Reverse`, `Sort`, `First`, `Rest`, `Partition`, `arrayload`/`arraystore`. |
| `sli_container_ops.cpp` | Polymorphic container ops: `length`, `get`, `put`, `empty`, `append`, `prepend`, `join`, `search`, `cva`, dict helpers. |
| `sli_io_ops.cpp` | I/O: `ifstream`, `ofstream`, `close`, `eof`, `getline_is`. |
| `sli_startup.cpp` | Bootstrap: `statusdict`, `cin`/`cout`/`cerr`, `<-`, `<--`, `evalstring`. |
| `sli_builtins.cpp` | Catch-all registration when nothing else fits. |

If you're adding *another arm* to an existing polymorphic op (e.g.
making `empty` accept a new type), edit the dispatcher in
`sli_container_ops.cpp`. If you're adding a *new* operator, start
in whichever file owns its semantic neighbours.

---

## 2. The skeleton

Every operator is a `SLIFunction` subclass with one virtual
method:

```cpp
class FooFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        // 1. validate
        // 2. work
        // 3. update stacks
    }
};
```

Then one global instance in the anonymous namespace, and one
`createcommand` call in the file's `init_*` function:

```cpp
FooFunction foo_fn;

// inside init_<area>(SLIInterpreter* i):
i->createcommand("foo", &foo_fn);
```

That's it for the simplest case. The next sections cover what
goes in the three numbered steps.

---

## 3. Stack-handling contract

The full rule set is in `CLAUDE.md` under "Stack-handling
discipline." The TL;DR an operator author has to internalise:

1. **Validate first, mutate last.** Stacks must reflect the call
   site at the moment `raiseerror` fires so `stopped`/`stop` can
   recover. Run every `require_stack_*` and range check before any
   `pop`, `push`, or in-place mutation.
2. **Peek with `pick(n)` / `top()`, don't `pop` to read.** Only
   pop once the work is committed.
3. **Don't touch your own e-stack slot.** Under new-ABI (the
   default and only ABI today — see §5) the dispatcher pre-pops
   the operator before calling `execute()`. An operator that calls
   `EStack().pop()` will pop the *caller's* frame and corrupt the
   estack.
4. **For raw-assigned pointer-payload Tokens, call
   `add_reference()` after the assignment.** Prefer the
   `new_token<TID>(payload)` factories; they get this right.
   Missed `add_reference()` is the canonical use-after-free in
   this codebase — see `DictionaryStack::toArray` (commit
   referenced in CLAUDE.md) for the failure mode.

The runtime checks the contract loosely (asserts in debug,
nothing in release). Bench-driven regressions surfaced two
violations during the 2026-05-11 recordstacks investigation; the
contract is now load-bearing.

---

## 4. Validation helpers

`SLIInterpreter` exposes terse validation primitives that raise
the right error and return without further work:

```cpp
i->require_stack_load(N);                // ensures load >= N
i->require_stack_type(slot, sli3::T);    // type-tag check
```

Both raise (`StackUnderflow` / `ArgumentType`) and the dispatcher
unwinds — your code below the call won't execute. If you need
more nuanced checks, raise yourself:

```cpp
if (k < 0 || static_cast<size_t>(k) >= arr->size())
{
    i->raiseerror(i->RangeCheckError);
    return;
}
```

The `i->XxxError` Names (`ArgumentTypeError`, `RangeCheckError`,
`StackUnderflowError`, `UndefinedNameError`, etc.) are declared
on `SLIInterpreter` — use them rather than constructing a `Name`
ad hoc, so error attribution is consistent.

---

## 5. The ABI (new-ABI is the only ABI now)

Every operator runs under the post-Axis-I "new-ABI" contract:

- The dispatcher pre-pops the operator's frame before calling
  `execute()`.
- The body therefore must **not** call `i->EStack().pop()` to
  pop itself.
- Control-flow ops that push *new* frames (the iter machinery,
  `if`/`ifelse` pushing branches) still do so as part of their
  semantics; they just don't pop themselves.

`set_new_abi()` survives as a no-op marker (see
`sli_function.h:114`). Calling it on new ops is purely
documentation — no runtime effect. If you don't, nothing breaks.

---

## 6. Constructing result Tokens

Three patterns, in order of preference:

```cpp
// Preferred: typed factory. Handles type tag + refcount.
i->push(i->new_token<sli3::arraytype>(my_arr));
i->push(i->new_token<sli3::integertype, long>(42));

// In-place push of a scalar — same factory under the hood.
i->push<bool>(true);
i->push<long>(n);

// Raw assignment — only when no factory exists for the
// (typeid, payload) pair. Must add_reference() after.
Token t(i->get_type(sli3::dictionarytype));
t.data_.dict_val = d;
t.add_reference();
i->push(t);
```

The `new_token<TID, Payload>` specialisations live in
`sli_interpreter.h:560-960`. For pointer-payload types, the
factory either consumes a freshly-allocated `refs_=1` heap object
(no extra `add_reference` needed) or copies an existing one
(handles the bump). Mixing models — passing an existing pointer
to a factory that expects a fresh allocation — leaks the original
reference. Read the relevant specialisation if in doubt.

Reinterpreting a Token's type *without* changing payload (e.g.
proc → array, since they share `TokenArray*`) is a single line:

```cpp
i->top().type_ = i->get_type(sli3::arraytype);
```

The refcount is on the heap object, not the `SLIType`, so
flipping `type_` is safe as long as the source and target types
share the same payload union slot. See `CvaFunction::execute` and
`ForallFunction::execute` for examples.

---

## 7. Multi-type dispatch

When one SLI name covers multiple operand types (`/length` works
on arrays, strings, dicts; `/get` on arrays, strings, dicts,
procs, litprocs), the pattern is a thin C++ dispatcher that
switches on the operand's tag and delegates to typed leaves:

```cpp
void EmptyFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(1);
    switch (i->top().tag()) {
      case sli3::arraytype:      empty_a_fn.execute(i); return;
      case sli3::dictionarytype: empty_d_fn.execute(i); return;
      case sli3::stringtype:     empty_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}
```

The typed leaves (`empty_a_fn`, `empty_d_fn`, `empty_s_fn`) are
themselves registered (`createcommand("empty_a", &empty_a_fn)`
etc.) so SLI code can call them directly when type is known.

When the same payload shape (TokenArray) recurs across multiple
typeids, template the leaf instead of duplicating:

```cpp
template <unsigned int TID>
class LengthArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, TID);
        long n = static_cast<long>(i->top().data_.array_val->size());
        i->pop();
        i->push<long>(n);
    }
};

LengthArrayLikeFunction<sli3::arraytype>        length_a_fn;
LengthArrayLikeFunction<sli3::proceduretype>    length_p_fn;
LengthArrayLikeFunction<sli3::litproceduretype> length_lp_fn;
```

This is the dominant pattern for the array/proc/litproc family in
`sli_container_ops.cpp`.

---

## 8. Hot-path inlining via `sli_op_bodies.h`

For operators the dispatcher sees in tight loops (`add`, `pop`,
`dup`, `get`, `put`, `if`, `def`), the body lives twice:

- As `hot_op_xxx(SLIInterpreter*)` in
  `src/builtins/sli_op_bodies.h` (static inline).
- As a `case` arm in `execute_dispatch_`'s big switch, which
  calls the same helper — the compiler inlines it into the
  switch.

The corresponding `SLIFunction::execute()` is a one-liner that
also calls the helper, so the slow path (operator invoked through
`functiontype`) and the fast path (dispatched directly off the
typeid) share one source. See `GetFunction::execute` and
`hot_op_get` for the canonical pair.

You only need to do this for genuinely hot ops. The catalog in
`doc/hot_ops.md` lists which ones qualify and why; everything
else stays purely in its `.cpp` file. Adding a new helper to
`sli_op_bodies.h` requires nothing of the dispatcher unless you
also wire it into the switch — which is a deliberate, separate
step.

---

## 9. Registration

In the file's `init_<area>(SLIInterpreter* i)` function (called
once during startup from `sli_interpreter.cpp`):

```cpp
void init_sliarray(SLIInterpreter* i)
{
    i->createcommand("foo",   &foo_fn);
    i->createcommand("foo_",  &foo_alt_fn);  // see §10
}
```

`createcommand(name, fn)` interns `name` and binds it in
`systemdict` as a `functiontype` Token pointing at `fn`. The
pointer is stored permanently — instances must outlive the
interpreter (the anonymous-namespace globals in each `.cpp`
qualify).

---

## 10. When SLI also defines your name

Some operators are defined *both* in C++ (for speed) and in SLI
(for portability, or because they predate the C++ implementation).
The vendored startup scripts in `lib/sli/` execute after
`init_*` runs, so a same-named SLI def will shadow the C++
binding.

The convention is the `bind_` pattern:

1. Register the C++ leaf with a `_` suffix:
   `i->createcommand("bind_", &bindfunction);`
2. In `lib/sli/sli-init.sli`, alias the public name to the C++
   leaf:
   ```
   /bind /bind_ load def
   ```

`First` / `Rest` (commit `73cae74`) and `bind` are the live
examples. The cost is one indirection on lookup; the win is that
the public name is owned by the SLI file (one source of truth)
and the C++ implementation can change names freely.

If the public SLI def has wider type coverage than your C++ leaf
(e.g. the old `/First { 0 get }` accepted strings via `get`'s
polymorphism), you're either narrowing the contract on purpose
(document it in the commit) or you need to keep a SLI dispatcher
that delegates to your leaf for the common case and falls back
for the rest.

---

## 11. Tests

Drop `test_<thing>.cpp` next to the others under `tests/` and add
a short block to `CMakeLists.txt`:

```cmake
add_executable(test_foo tests/test_foo.cpp)
target_link_libraries(test_foo PRIVATE sli3_core)
add_test(NAME test_foo COMMAND test_foo)
```

No external framework — bare `assert(...)` (per Q8 in
`implementation_spec.md`). For ops that take SLI input,
`tests/test_harness.h` lets you feed source through the
dispatcher (`test_sli_eval.cpp` is the model).

Run with `ctest --test-dir build` before claiming the slice is
done.

---

## 12. Worked example: `/Last`

Adding `Last` (return the last element of an array/proc/litproc)
end to end.

**Decide where:** `sli_array_module.cpp` — it's an array stdlib
op, neighbour of `First` and `Rest`.

**Write the class** (after the `Rest` definition):

```cpp
class LastFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        unsigned tag = i->top().tag();
        if (tag != sli3::arraytype
            && tag != sli3::proceduretype
            && tag != sli3::litproceduretype)
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        TokenArray const* arr = i->top().data_.array_val;
        if (arr->empty())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        // Copy element out before overwriting top() — otherwise
        // the assignment drops the array's last reference and
        // frees the storage we're reading from.
        Token last = arr->get(static_cast<long>(arr->size() - 1));
        i->top() = std::move(last);
    }
};
```

**Add the instance + registration** (alongside `first_fn` /
`rest_fn`):

```cpp
LastFunction last_fn;

// inside init_sliarray:
i->createcommand("Last_", &last_fn);
```

**Alias from SLI** (`lib/sli/sli-init.sli`, replacing the
existing `/Last { size 1 sub_ii get } bind def`):

```
% /Last is a C++ leaf in sli_array_module: LastFunction.
% Handles array/proc/litproc.
/Last /Last_ load def
```

**Build and verify:**

```sh
cmake --build build -j
ctest --test-dir build
echo '[1 2 3] Last == { 10 20 } Last == [] Last' | ./build/sli3
```

Expect `3`, `20`, then a `RangeCheck` on the empty array.

That's the full path: one class, one instance, one
`createcommand`, one SLI alias, and a smoke test. Anything more
than that — multi-type dispatch, hot-path inlining, templated
leaves — is layered on top of this base.

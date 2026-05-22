# 10. Three small programs

This final chapter puts everything together. Each program uses only
operators introduced in earlier chapters; the goal is to show what
idiomatic SLI looks like for problems slightly larger than a one-liner.

Each example has a sibling `.sli` file under
`examples/ch10_full/` that you can run directly:

```
./build/sli3 doc/tutorial/examples/ch10_full/factorial.sli
```

## 1. Recursive factorial

```sli
/factorial {
  dup 1 leq
  { pop 1 }
  { dup 1 sub factorial mul }
  ifelse
} def

5 factorial =
120

10 factorial =
3628800
```

Read the procedure body as: duplicate the input. If it is `≤ 1`, pop
the input and push `1`. Otherwise duplicate it again, subtract one,
recurse, then multiply. The recursion bottoms out at `1`.

A few details worth noticing:

- `pop 1` in the `then` branch discards the input and pushes the
  literal `1`. We need the discard because the comparison already
  duplicated the original — `dup 1 leq` consumes one copy of the input
  but leaves the original in place.
- `factorial mul` works because the recursive call leaves its result
  on the stack; `mul` then combines it with the `dup`'d copy of the
  current `n`.
- There is no special syntax for recursion. The body refers to
  `factorial` by name; the executable name resolves via the dictstack
  at each invocation.

## 2. Summing an array

```sli
/sum {
  0 exch
  { add } forall
} def

[1 2 3 4 5] sum =
15

[10 20 30] sum =
60
```

`sum` expects an array on top of stack. The body pushes `0` (the
accumulator), then `exch` swaps it under the array. `forall` runs
`{ add }` once per element: each iteration adds the current element to
the running total. After `forall` completes, only the total remains.

This is a small but important pattern: the `0 exch { add } forall`
idiom is to RPN what `sum(arr)` is to Python.

To compute a product instead, change the initial value and the
operator:

```sli
/product { 1 exch { mul } forall } def

[1 2 3 4 5] product =
120
```

## 3. Dictionary dispatch

A common shape in SLI programs is to keep a dictionary of named
actions and look them up at runtime. Each entry is a procedure; the
caller picks one by name.

```sli
/dispatch
<<
  /greet { (Hello!) = }
  /bye   { (Goodbye!) = }
  /shout { (HEY!) = }
>> def

/run { dispatch exch get exec } def

/greet run
Hello!

/bye run
Goodbye!
```

`/run` expects a literal name on top of the stack. It pushes the
dispatch table, swaps so the name is on top of the table, uses `get`
to find the matching value (the procedure), and `exec`'s it.

To add a new command you just put another entry in `dispatch`:

```sli
dispatch /hum { (Mmmm.) = } put
/hum run
Mmmm.
```

This is the same shape as a Python `dict[str, Callable]` table or a
C struct of function pointers. SLI does it with one literal and three
operators.

## Where to go next

You now know enough SLI to read most of the standard library scripts
under `lib/sli/` — start with `lib/sli/sli-init.sli` which loads at
startup and defines a lot of the helper operators you have been using.
The benchmark programs under `bench/sli/` are also short and focused.

For a comprehensive operator reference, see
[`core_commands.md`](core_commands.md) (the beginner set) or grep the
sources under `src/builtins/` and `lib/sli/` for the full list of
~300 operators. Every operator has a `Name:` line in its docstring;
many have an `Examples:` section.

The biggest topics not covered in this tutorial:

- **Error handling** with `stopped` / `stop` / `$errordict`.
- **File and string I/O** with `ifstream`, `ofstream`,
  `getline_is`, `ostrstream`.
- **Access modes** (`readonly`, `executeonly`, `noaccess`) that lock
  containers against modification or inspection.
- **Type tries** — the polymorphic dispatch machinery behind operators
  like `add` that work on integers, doubles, and arrays.
- **Serialization** of values to/from disk.

Each of these is a chapter's worth of material in its own right; the
references above are the places to start reading.

---

Back to the [table of contents](README.md).

# 06. Arrays

An array in SLI is indexable, mutable sequence of tokens.
The elements can be of any type, including other arrays.

## Array literals

To produce an array, surround a list of tokens with square brackets:

```
[1 2 3]
```

The parser reads `[`, collects every token until the matching `]`, and
constructs an array. Each element is pushed and popped during
construction, so anything that runs between the brackets can affect
the contents:

```
[1 2 add 7 8 mul] ==
[3 56]
```

`1 2 add` ran and pushed `3`; `7 8 mul` ran and pushed `56`; the
closing bracket collected the two results.

Arrays can mix types:

```
[1 (two) 3.0 /four] ==
[1 (two) 3 /four]
```

`==` prints arrays in syntax form (brackets, with string parens and
literal-name slashes preserved); `=` would print elements separated by
spaces with no decoration. The chapter on printing covers the
difference in detail.

## Length

`length` `(arr -- n)` returns the element count.

```
[10 20 30] length =
3

[] length =
0
```

`length` also works on strings.

## Indexing

`get` `(arr i -- value)` returns the element at index `i` (0-based).

```
[10 20 30 40] 2 get =
30
```

`put` `(arr i value -- arr')` returns the array with element `i`
replaced. The original array is mutated in place; the array on the
stack is the (mutated) same array.

```
[10 20 30] 1 99 put ==
[10 99 30]
```

Both `get` and `put` raise a `RangeCheck` error if the index is out of
bounds. Negative indices are not supported.

## Adding and removing elements

`append` `(arr value -- arr')` adds an element to the end.

```
[1 2 3] 4 append ==
[1 2 3 4]
```

`prepend` `(arr value -- arr')` adds to the front.

```
[2 3 4] 1 prepend ==
[1 2 3 4]
```

`join` `(a b -- c)` concatenates two arrays.

```
[1 2] [3 4] join ==
[1 2 3 4]
```

`join` is polymorphic: applied to two strings it concatenates them too.

## First, Rest, and friends

`First` `(arr -- value)` returns the first element. `Rest` `(arr -- arr')`
returns the array with the first element removed.

```
[10 20 30] First =
10

[10 20 30] Rest ==
[20 30]
```

Empty arrays have neither a `First` nor a meaningful `Rest`; both raise
an error.

## Constructing arrays from scratch

`array` `(n -- arr)` creates an array of `n` zeros. Useful for
preallocating storage.

```
5 array ==
[0 0 0 0 0]
```

`Range` produces an integer range. The syntax differs from most
languages — it takes its bounds as an array, not as separate stack
elements:

```
[5] Range ==
[1 2 3 4 5]

[2 6] Range ==
[2 3 4 5 6]

[1 10 2] Range ==
[1 3 5 7 9]
```

The three forms are: a single endpoint (`[N] -> 1..N`), two endpoints
(`[A B] -> A..B`), and three values (`[start end step]`).

## Iterating an array

The chapter on loops covered `forall` and `forallindexed`. Recapping:

```
0 [1 2 3 4 5] { add } forall =
15
```

## `Map` — transform each element

For element-by-element transformation without writing a `forall`
yourself, the standard library provides `Map`. It takes an array and a
procedure; the procedure runs once per element with the element on the
stack, and is expected to leave exactly one value behind. `Map`
collects those values into a new array of the same length.

```
[1 2 3 4] { dup mul } Map ==
[1 4 9 16]
```

The body sees one element at a time and must leave one result:

```
[10 20 30] { 100 add } Map ==
[110 120 130]

[(red) (green) (blue)] { length } Map ==
[3 5 4]
```

An empty input gives an empty output:

```
[] { 1 add } Map ==
[]
```

`Map` is the right tool when you want a transformed copy of an array.
Compare with `forall`, which runs the body for side effects (or for an
accumulator left on the stack) but does not build a result array.

## `MapThread` — zip several arrays together

`MapThread` `([[r1] [r2] ...] proc -- arr)` takes an outer array
whose entries are equal-length inner arrays, and walks them in
lockstep. On each step it pushes one element from each row at the
same column index, then runs the body, which must leave exactly one
value behind. The collected values form the result array, one per
column.

Elementwise add of two vectors:

```
[[1 2 3] [10 20 30]] { add } MapThread ==
[11 22 33]
```

`MapThread` is the SLI analogue of Python's `zip(...)` combined with a
list comprehension — given parallel arrays, run a body once per
column.

Three rows work the same way, with three values on the stack per
iteration:

```
[[1 2 3] [10 20 30] [100 200 300]] { add add } MapThread ==
[111 222 333]
```

Zip pairs into 2-element arrays using `arraystore`:

```
[[1 2 3] [4 5 6]] { 2 arraystore } MapThread ==
[[1 4] [2 5] [3 6]]

[[/a /b /c] [1 2 3]] { 2 arraystore } MapThread ==
[[/a 1] [/b 2] [/c 3]]
```

The inner arrays must all have the same length — `MapThread` raises
`RangeCheck` if they do not — and the outer array must contain only
arrays (else `ArgumentType`).

`Map` and `MapThread` are implemented as native C++ operators (with
trie dispatch wired up in `lib/sli/mathematica.sli`); they are loaded
by default and behave like any other SLI operator. There is also
`MapIndexed`, which works like `Map` but pushes the 1-based index
alongside each element: `[10 20 30] { add } MapIndexed -> [11 22 33]`.

## Examples

- [`examples/ch06_arrays/literal.sli`](examples/ch06_arrays/literal.sli)
- [`examples/ch06_arrays/length_get.sli`](examples/ch06_arrays/length_get.sli)
- [`examples/ch06_arrays/append_join.sli`](examples/ch06_arrays/append_join.sli)
- [`examples/ch06_arrays/range_map.sli`](examples/ch06_arrays/range_map.sli)
- [`examples/ch06_arrays/map_mapthread.sli`](examples/ch06_arrays/map_mapthread.sli) — `Map` transforms, `MapThread` zips.

---

Next: [chapter 07 — dictionaries](07_dicts.md).

# 06. Arrays

An array in SLI is an ordered, indexable, mutable sequence of tokens.
The elements can be of any type, including other arrays — arrays nest.

## Array literals

Surround a list of tokens with square brackets:

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
returns the array with the first element removed. These are
SLI-defined helpers (not built-in C++ operators) that come from
`lib/sli/sli-init.sli`.

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

For element-by-element transformation without writing a `forall`
yourself, the standard library provides `Map`. It takes an array and a
procedure; the procedure runs once per element with the element on the
stack, and `Map` collects the result.

```
[1 2 3 4] { dup mul } Map ==
[1 4 9 16]
```

Map is part of `lib/sli/mathematica.sli` rather than the built-in core,
but it is loaded by default and behaves like any other SLI operator.

## Examples

- [`examples/ch06_arrays/literal.sli`](examples/ch06_arrays/literal.sli)
- [`examples/ch06_arrays/length_get.sli`](examples/ch06_arrays/length_get.sli)
- [`examples/ch06_arrays/append_join.sli`](examples/ch06_arrays/append_join.sli)
- [`examples/ch06_arrays/range_map.sli`](examples/ch06_arrays/range_map.sli)

---

Next: [chapter 07 — dictionaries](07_dicts.md).

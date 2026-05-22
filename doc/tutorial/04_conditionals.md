# 04. Conditionals

SLI has a single boolean type and three branching operators. This
chapter walks through both.

## Booleans

`true` and `false` are predefined names. Pushing them pushes a boolean
value:

```
true type =
/booltype

true =
true
```

The comparison operators take two numbers and push a boolean.

## Numerical comparison

`eq`, `neq`, `gt`, `lt`, `geq`, `leq` all have stack effect
`(num num -- bool)`.

```
3 3 eq =
true

3 4 eq =
false

3 4 lt =
true

5 5 leq =
true

5 5 lt =
false
```

`eq` and `neq` also work on names and strings:

```
/foo /foo eq =
true

(hello) (world) eq =
false
```

`gt`, `lt`, `geq`, `leq` only work on numbers.

## Boolean operators

`and`, `or`, `not` operate on booleans. `and` and `or` take two and
return one; `not` takes one and returns one.

```
true false and =
false

true false or =
true

true not =
false
```

These three operators also work on integers, where they act bit-by-bit:

```
12 10 and =
8

12 10 or =
14
```

Stick to booleans unless you mean to do bit manipulation.

## `if` — one-way branch

`if` `(bool proc -- )` runs `proc` when `bool` is `true` and does
nothing when it is `false`:

```
1 0 gt { (positive) = } if
positive
```

The procedure body is `{ (positive) = }`. `(positive)` is a string;
`=` pops it from the stack and prints it.

When the condition is false, nothing happens:

```
0 1 gt { (positive) = } if
```

(no output)

## `ifelse` — two-way branch

`ifelse` `(bool then-proc else-proc -- )` runs the first procedure on
`true` and the second on `false`:

```
5 0 gt
{ (positive) = }
{ (non-positive) = }
ifelse
positive
```

The procedures can leave values on the stack instead of printing:

```
3 4 lt { (yes) } { (no) } ifelse =
yes
```

Both branches push a string; `ifelse` runs one of them; then `=` prints
whichever string was left.

## Combining comparisons

You can build compound conditions by combining comparisons with `and`,
`or`, `not`:

```
/x 7 def
x 0 gt   x 10 lt   and
{ (in range) = }
{ (out of range) = }
ifelse
in range
```

Read the third line as "`x` greater than `0` AND `x` less than `10`".
Both comparisons push a boolean; `and` pops both and pushes their
conjunction.

## Pitfall: side-effects in branches

In SLI a procedure can both leave a value on the stack and print
something. The chapter on printing covers `=` and other output
operators in more detail; for now just be aware that mixing them inside
branches can be surprising.

```
3 4 lt { 1 = } { 0 = } ifelse
1
```

This prints `1` and leaves nothing on the stack — `=` consumed the
value. Compare with:

```
3 4 lt { 1 } { 0 } ifelse =
1
```

Same printed output; this version leaves the result of `ifelse` on the
stack for `=` to consume.

## Absolute-value example with `ifelse`

The built-in `abs` already does this, but writing it out shows the
shape of a typical `ifelse`:

```
/myabs { dup 0 lt { neg } { } ifelse } def
-5 myabs =
5
7 myabs =
7
```

The `else` branch is the empty procedure `{ }` because if the input is
already non-negative we want to leave it alone.

## Examples

- [`examples/ch04_conditionals/compare.sli`](examples/ch04_conditionals/compare.sli)
- [`examples/ch04_conditionals/if_ifelse.sli`](examples/ch04_conditionals/if_ifelse.sli)
- [`examples/ch04_conditionals/myabs.sli`](examples/ch04_conditionals/myabs.sli)

---

Next: [chapter 05 — loops](05_loops.md).

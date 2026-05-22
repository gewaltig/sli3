# 08. Types and conversion

This chapter covers `type` (find out what something is) and the family
of `cv...` operators (convert between types).

## Asking the type

`type` `(any -- /name)` pushes the type-name of the object on top of
stack. The result is a literal name like `/integertype` or
`/stringtype`.

```
42 type =
/integertype

3.14 type =
/doubletype

true type =
/booltype

(hello) type =
/stringtype

/foo type =
/literaltype

[1 2 3] type =
/arraytype

<< /a 1 >> type =
/dictionarytype

{ 1 add } type =
/proceduretype
```

The type name follows a predictable pattern: the readable name with
`type` appended. The full list is the `sli_typeid` enum in
`src/types/sli_token.h`; the ones above are the types you will see day
to day.

## Converting numbers

`cvi` `(num -- int)` converts to integer. Doubles are truncated toward
zero.

```
3.9 cvi =
3

-3.9 cvi =
-3
```

`cvd` `(num -- double)` converts to double.

```
5 cvd =
5

5 cvd type =
/doubletype
```

The first looks like it does nothing, but `type` confirms the
conversion happened — the runtime now stores a double even though the
printed form is identical at six significant figures.

`cvi` can also parse a string:

```
(42) cvi =
42

(3.9) cvi =
3
```

`cvd` parses doubles from strings similarly:

```
(3.14) cvd =
3.14
```

If the string cannot be parsed, you get an error.

## Converting to and from strings

`cvs` `(any -- string)` produces a printable string. The result is a
plain string you can manipulate or pass to other operators.

```
42 cvs ==
(42)

3.14 cvs ==
(3.14)

/foo cvs ==
(/foo)
```

For numbers, `cvs` produces the same characters that `=` would print.
For literal names, the leading slash is kept — the resulting string is
the same syntactic form you would type to push the literal name.

## Predicate-style checks

When you want a conditional based on the type, the idiomatic form is to
compare the result of `type` against a literal:

```
42 type /integertype eq =
true

(hi) type /integertype eq =
false
```

This is how the SLI standard library checks argument types before
diving into a computation.

## Why convert at all?

In a stack language with no static types, conversions matter mainly
when reading user input (strings → numbers), writing output (numbers →
strings for formatting), and bridging operators that expect a specific
type. You will see `cvi` and `cvd` most often near `getline_is` (which
reads a line as a string) and `==`/`=only` (which write).

## Examples

- [`examples/ch08_types/type.sli`](examples/ch08_types/type.sli)
- [`examples/ch08_types/conversions.sli`](examples/ch08_types/conversions.sli)

---

Next: [chapter 09 — printing](09_printing.md).

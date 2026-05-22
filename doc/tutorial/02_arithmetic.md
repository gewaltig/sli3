# 02. Arithmetic

SLI has two numeric types: **integer** (a 64-bit signed integer) and
**double** (a 64-bit IEEE float). Most arithmetic operators are
polymorphic: they pick integer or double behaviour from the types of
their operands. Mixed operations widen to double.

## The four arithmetic operators

`add`, `sub`, `mul`, `div` all take two numbers and push one number.
Stack effect: `(num num -- num)`.

```
3 4 add =
7

10 4 sub =
6

6 7 mul =
42

20 4 div =
5
```

Integer division produces an integer:

```
7 2 div =
3
```

To force floating-point division, make at least one operand a double:

```
7 2.0 div =
3.5
```

`div` by zero raises an error. Try it:

```
1 0 div
May 22 11:34:00 SLIInterpreter::execute [M_ERROR]: DivisionByZero
```

The interpreter prints an error message and unwinds back to the prompt;
the stack is left in the state it had at the error site (the `1` and
`0` are still there). Chapter 09 covers what happens during an error in
more detail.

## Modulo

`mod` `(int int -- int)` is integer modulo. Both operands must be
integers.

```
7 4 mod =
3

100 7 mod =
2
```

## Negation and absolute value

`neg` `(num -- num)` negates a single number. `abs` `(num -- num)`
returns absolute value. Both are polymorphic — they work on integers and
doubles.

```
5 neg =
-5

-3.5 abs =
3.5
```

`neg` is useful inside RPN because there is no minus prefix for
literals — `-5` is a literal, but `0 5 sub` and `5 neg` are equivalent
ways to produce `-5` from a positive `5`.

## Roots and powers

`sqrt` `(num -- double)` is the square root.

```
16 sqrt =
4

2.0 sqrt =
1.41421
```

`sli3` prints doubles with six significant figures by default. The full
double is still stored; only the printed form is truncated.

`pow` `(base exp -- num)` raises `base` to `exp`. If both are integers
the result is an integer; otherwise it is a double.

```
2 10 pow =
1024

2.0 0.5 pow =
1.41421
```

## Order matters

In RPN, the first operand pushed is the first one popped. For non-
commutative operators that means the subtrahend or divisor goes second:

```
10 3 sub =     % 10 - 3
7

3 10 sub =     % 3 - 10
-7
```

A helpful way to read `a b sub` is "a minus b". The thing on the right
of the operator is the bottom of the pair on the stack; the thing
furthest right (in textual order) is the top.

## A small worked example

The Pythagorean theorem: given two sides of a right triangle, compute
the hypotenuse.

```
3 dup mul 4 dup mul add sqrt =
5
```

Reading it left to right: push `3`, duplicate, multiply (so `9` is on
the stack); push `4`, duplicate, multiply (now `9` and `16`); add
(`25`); square root (`5.0`). The promotion to double happens at
`sqrt`.

In chapter 03 you will define this as a procedure named `hypot` so you
do not have to write it out every time.

## Examples

- [`examples/ch02_arithmetic/basic.sli`](examples/ch02_arithmetic/basic.sli)
- [`examples/ch02_arithmetic/order.sli`](examples/ch02_arithmetic/order.sli)
- [`examples/ch02_arithmetic/pythagoras.sli`](examples/ch02_arithmetic/pythagoras.sli)

---

Next: [chapter 03 — names and procedures](03_names.md).

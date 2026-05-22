# 05. Loops

SLI has four looping operators. Each takes a procedure and runs it
repeatedly under some condition.

## `repeat` — run N times

`repeat` `(n proc -- )` runs `proc` `n` times.

```
3 { (hi) = } repeat
hi
hi
hi
```

The procedure does not see an index; if you need one, use `for`.

A common use of `repeat` is to accumulate a value:

```
0 5 { 1 add } repeat =
5
```

Start with `0` on the stack, then `5` and the proc `{ 1 add }`.
`repeat` pops `5` and the proc, leaving `0` on the stack, then runs the
proc 5 times. Each iteration adds 1. The final value is printed.

## `for` — counted loop with index

`for` `(start step stop proc -- )` runs `proc` for an index that
starts at `start`, advances by `step` each iteration, and stops when it
would exceed `stop`. Each iteration pushes the current index onto the
stack before running `proc`.

```
1 1 5 { = } for
1
2
3
4
5
```

`=` here prints the index `for` pushed onto the stack.

You can use a negative step to count down:

```
10 -2 0 { = } for
10
8
6
4
2
0
```

The bench scripts use `for` heavily for index-driven loops; you will see
this idiom often.

## `loop` and `exit` — unbounded loop

`loop` `(proc -- )` runs `proc` forever. The only way out is `exit`,
which terminates the innermost active loop.

```
0
{ dup 5 geq { exit } if 1 add } loop
=
5
```

Read the body: duplicate the counter; if it is `>= 5`, exit; otherwise
add `1`. The `=` after the closing brace runs once the loop has
exited, and prints the accumulated value.

`loop`/`exit` is the right tool when the termination condition is not a
simple counter — for instance, when you read until end-of-input. For
counted iteration, prefer `repeat` or `for`.

## `forall` — iterate over an array

`forall` `(array proc -- )` runs `proc` once for each element of an
array, pushing the element onto the stack before each call.

```
[10 20 30] { = } forall
10
20
30
```

To sum an array:

```
0 [1 2 3 4 5] { add } forall =
15
```

The accumulator `0` is pushed, then the array and the proc. `forall`
runs `{ add }` once per element: each iteration pops the running sum
and the next element, pushes their sum. The final sum is on the stack
after `forall` finishes.

## `forallindexed` — iterate with index

`forallindexed` `(array proc -- )` is like `forall` but pushes both the
element and the element's index. After the push the stack ends with the
element below and the index on top:

```
[(red) (green) (blue)] { pstack clear } forallindexed
0
(red)
1
(green)
2
(blue)
```

`pstack` prints top-down. So on the first iteration the stack from top
to bottom is `0, (red)` — the index is on top, the element is below it.
That ordering is what you have to remember when writing the body.

To print "index" then "element" once per iteration, two `=` calls
suffice:

```
[(red) (green) (blue)] { = = } forallindexed
0
red
1
green
2
blue
```

The first `=` pops and prints the index; the second pops and prints the
element.

## Returning a value from a loop

Loops do not have a "return value" the way Python functions do — they
just leave whatever they leave on the stack. The accumulator pattern
(see `repeat` and `forall` above) is the usual way to compute one.

## Examples

- [`examples/ch05_loops/repeat.sli`](examples/ch05_loops/repeat.sli)
- [`examples/ch05_loops/for.sli`](examples/ch05_loops/for.sli)
- [`examples/ch05_loops/loop_exit.sli`](examples/ch05_loops/loop_exit.sli)
- [`examples/ch05_loops/forall.sli`](examples/ch05_loops/forall.sli)

---

Next: [chapter 06 — arrays](06_arrays.md).

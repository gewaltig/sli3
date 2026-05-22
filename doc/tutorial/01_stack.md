# 01. The stack

SLI has no expression parser. There is no `1 + 1`; there is `1 1 add`.
The reason is that SLI keeps a stack of values, and every operator gets
its arguments by popping the stack and returns its result by pushing onto
the stack. The notation that goes with this model is called **Reverse
Polish Notation** (RPN).

If you are used to writing `add(1, 1)`, RPN is the same idea with the
function name moved to the right of the arguments. Where Python would
write `(2 + 3) * 4`, SLI writes `2 3 add 4 mul`. There are no
parentheses, no precedence rules, and no special syntax for nested
calls.

## Pushing values

Anything that is not an operator is a value, and values get pushed onto
the operand stack as they are read.

```
1 2 3
```

After this line the operand stack has three integers on it. The top of
stack is `3` (the most recently pushed). To see the stack, use the
`stack` operator:

```
1 2 3 stack
3
2
1
```

`stack` prints the stack top-down without popping anything. The state is
unchanged afterwards.

To pop and print one value, use `=`:

```
1 2 3 =
3
```

After running this the stack has `1` and `2` left, with `2` on top.

## The eight stack operators

Most SLI code does its real work with arithmetic, control flow, and
container operators. But every SLI program is also shuffling values on
the stack. The following operators do nothing except move values around.

### `dup` — duplicate

`dup` `(a -- a a)` copies the top of stack.

```
5 dup add =
10
```

The first `dup` pushes a second `5`; `add` pops both and pushes `10`; `=`
prints it.

### `pop` — discard

`pop` `(a -- )` discards the top of stack.

```
1 2 3 pop =
2
```

We push 1, 2, 3, pop the 3 and discard it, then print 2.

### `exch` — swap top two

`exch` `(a b -- b a)` swaps the two top elements.

```
1 2 exch =
1
```

After `1 2`, the stack reads `1, 2` bottom to top. `exch` produces
`2, 1`. `=` pops and prints `1`.

### `over` — copy second

`over` `(a b -- a b a)` copies the element second from the top onto the
top.

```
3 7 over =
3
```

We start with `3, 7` (top `7`); `over` copies the `3` to give `3, 7, 3`;
`=` prints and pops the new `3`.

### `count` — depth of the stack

`count` `( -- n)` pushes the current number of items on the stack.

```
10 20 30 count =
3
```

`count` does not modify the items it counts.

### `clear` — empty the stack

`clear` `( -- )` discards everything on the operand stack.

```
1 2 3 4 clear count =
0
```

After `clear`, `count` reports zero.

### `index` — copy n-deep

`index` `( ... a b c n -- ... a b c X)` where `X` is a copy of the
n-th element below the top of stack (`n=0` is the top, `n=1` is the
element below it).

```
100 200 300 2 index =
100
```

The `2` says "copy the value two below the top". The stack has `100`,
`200`, `300` before the `2`; two below `300` is `100`.

### `roll` — rotate a slice

`roll` `( ... an...a1 n j -- ... rotated)` takes the top `n` items and
rotates them by `j` positions. Positive `j` rotates toward the top;
negative `j` rotates toward the bottom.

```
1 2 3 3 1 roll stack
2
1
3
```

The `3 1` arguments say "take the top three items and rotate by one
position toward the top". `1 2 3` (bottom to top) becomes `3 1 2`,
which `stack` prints top-down as `2, 1, 3`.

## A small exercise

Compute `(a + b) * (a - b)` for `a = 7` and `b = 3` using only the
operators introduced so far. One way:

```
7 3 2 copy add 3 1 roll sub mul =
40
```

`2 copy` is a stack op we haven't introduced yet — it copies the top two
items as a block. The same result without `copy`:

```
7 dup 3 dup 3 1 roll add 3 1 roll sub mul =
40
```

This is messy on purpose. As you read SLI in the wild you will see
people work around stack shuffling by defining named variables — chapter
03 covers that. For now, get comfortable with the seven primitives
above; they let you do anything, even when defining names would be
cleaner.

## Examples

Each chapter has runnable snippets under `examples/chXX_topic/`. For
this chapter they are:

- [`examples/ch01_stack/dup.sli`](examples/ch01_stack/dup.sli)
- [`examples/ch01_stack/exch_over.sli`](examples/ch01_stack/exch_over.sli)
- [`examples/ch01_stack/count_clear.sli`](examples/ch01_stack/count_clear.sli)
- [`examples/ch01_stack/index_roll.sli`](examples/ch01_stack/index_roll.sli)

Run any of them with `./build/sli3 doc/tutorial/examples/ch01_stack/dup.sli`.

---

Next: [chapter 02 — arithmetic](02_arithmetic.md).

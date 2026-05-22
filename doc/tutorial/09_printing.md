# 09. Printing

Most output goes through one of two operator families: the high-level
`=` / `==` / `=only` set, which pop a value and write it; and the
low-level `<-` / `<--` set, which write to a stream you specify. This
chapter covers both, plus the two stack-inspection operators `stack`
and `pstack`.

## `=` — print and pop

`=` `(any -- )` pops the top of stack and prints it, followed by a
newline.

```
42 =
42

(hello) =
hello

/foo =
/foo

[1 (a) /b] =
[1 a /b]
```

Note the array example: the string `(a)` printed without its parens.
`=` strips the syntactic decoration that makes a value re-parseable.

## `==` — print in syntax form and pop

`==` `(any -- )` is the same but uses syntactic form. Strings get
their parens. The result is something you could paste back into the
interpreter:

```
(hello) ==
(hello)

[1 (a) /b] ==
[1 (a) /b]
```

Literal names print the same way under both `=` and `==`. Numbers print
the same way under both. The visible difference is mostly in strings
and arrays containing strings.

For everyday "show me the value" `==` is the safer choice; for
human-facing output `=` reads better.

## `=only` — no trailing newline

`=only` `(any -- )` is exactly like `=` but omits the final newline.

```
1 2 3 =only =only =only
321
```

This is how you build a single output line from several pieces:

```
/name (Alice) def
/age 30 def
(name=) =only name =only ( age=) =only age =
name=Alice age=30
```

A literal `==only` also exists; it does the same for `==`.

## `stack` — show the operand stack

`stack` `( -- )` prints the entire operand stack, top first, without
popping anything. Values use `=`-style printing.

```
1 (two) /three stack
/three
two
1
```

After this the stack still contains all three values. `stack` is the
basic interactive debugging tool.

## `pstack` — same, in syntax form

`pstack` `( -- )` is to `stack` what `==` is to `=`:

```
1 (two) /three pstack
/three
(two)
1
```

Notice the string `(two)` keeps its parens here.

## `<-` — write to a stream

The low-level write operators take an explicit stream and an object:

```
<- (stream any -- stream)
```

`<-` writes the object to the stream using `=`-style formatting and
leaves the stream on the stack for chaining. The standard output stream
is named `cout`.

```
cout (hello) <- endl
hello
```

`endl` `(stream -- stream)` writes a newline.

You can chain calls:

```
cout (the answer is ) <- 42 <- endl
the answer is 42
```

`<-` left the stream on the stack each time so the next call could use
it.

## `<--` — write to a stream in syntax form

`<--` is the syntax-form counterpart, same as `==` vs `=`:

```
cout (hello) <-- endl
(hello)
```

For interactive output you almost always want `=`/`<-` (no parens). For
serialisation, `==`/`<--` (syntactically re-readable form). 

## A common pattern

To print a formatted message that mixes a label and a computed value:

```
/x 7 def
cout (x is ) <- x <- endl
x is 7
```

Or with the high-level operators:

```
/x 7 def
(x is ) =only x =
x is 7
```

The high-level form is shorter; the stream form is more flexible when
you need to write to a file (chapter on I/O, not covered here).

## Examples

- [`examples/ch09_printing/eq_vs_eqeq.sli`](examples/ch09_printing/eq_vs_eqeq.sli)
- [`examples/ch09_printing/eqonly_line.sli`](examples/ch09_printing/eqonly_line.sli)
- [`examples/ch09_printing/stack_pstack.sli`](examples/ch09_printing/stack_pstack.sli)
- [`examples/ch09_printing/streams.sli`](examples/ch09_printing/streams.sli)

---

Next: [chapter 10 — three small programs](10_examples.md).

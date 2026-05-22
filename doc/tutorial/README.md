# SLI tutorial

A beginner's tour of SLI, the stack-based scripting language driven by
`sli3`. SLI is a small, fast, PostScript-inspired language that started
life as the scripting layer of the NEST neural simulator.

This tutorial is written for programmers who already know another
language — Python, C, JavaScript, Go, anything — and want to learn how
to read and write SLI. It does not assume any prior contact with
PostScript, Forth, or Reverse Polish Notation; chapter 01 introduces
those ideas from scratch. It does assume that you can already think in
loops, conditionals, and procedures.

## How to read it

Work through the chapters in order. Every code block is meant to be run.
Open `./build/sli3` in one terminal and paste examples in as you go. The
binary reads from stdin by default; lines you type are evaluated as soon
as you hit Enter.

```
$ ./build/sli3
SLI 1.0.0 (C) 2004 The NEST Initiative
 (C) 2026 Marc-Oliver Gewaltig

1 1 add =
2
```

Type `quit` to leave.

## Chapters

| # | File | Topic |
|---|------|-------|
| 00 | [Intro](00_intro.md) | What SLI is. Starting and stopping the interpreter. |
| 01 | [The stack](01_stack.md) | RPN. Pushing, popping, inspecting. |
| 02 | [Arithmetic](02_arithmetic.md) | Integer and double math. |
| 03 | [Names and procedures](03_names.md) | Binding names. Writing procedures. |
| 04 | [Conditionals](04_conditionals.md) | Comparisons, booleans, branching. |
| 05 | [Loops](05_loops.md) | `repeat`, `for`, `loop`, `forall`. |
| 06 | [Arrays](06_arrays.md) | Building and manipulating arrays. |
| 07 | [Dictionaries](07_dicts.md) | Key/value storage. The dictstack. |
| 08 | [Types and conversion](08_types.md) | Asking what something is. Converting between types. |
| 09 | [Printing](09_printing.md) | Writing output. Inspecting the stack. |
| 10 | [Three small programs](10_examples.md) | Factorial, summing an array, dictionary dispatch. |

## Reference

- [`core_commands.md`](core_commands.md) — every operator covered here,
  with source locations.
- [`examples/`](examples/) — runnable `.sli` files paired with the
  chapters. Each file is self-contained; run with
  `./build/sli3 doc/tutorial/examples/chXX_topic/name.sli`.

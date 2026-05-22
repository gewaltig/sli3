# 00. Intro

## What SLI is

SLI is a small scripting language. Its full name is **Simulation Language
Interpreter**, but the simulation part is no longer attached — `sli3` is a
standalone interpreter for the language. Programs are sequences of tokens
separated by whitespace; each token either pushes a value onto a stack or
runs an operator that consumes values from the stack and pushes new ones.
You will see no parentheses around function arguments and no operator
precedence to memorize. Everything is read left to right.

SLI is heavily inspired by PostScript. If you have used Forth or HP RPN
calculators, you already know the broad shape of the language. If you
have not, chapter 01 walks through the model from scratch.

## Building and starting `sli3`

The interpreter binary is `./build/sli3` after a build. From the repository
root:

```
cmake -S . -B build
cmake --build build -j
```

Start the interpreter with no arguments to get a read-eval loop on
standard input:

```
$ ./build/sli3
SLI 1.0.0 (C) 2004 The NEST Initiative
 (C) 2026 Marc-Oliver Gewaltig
```

The banner is printed every time. After the banner the interpreter waits
for input. There is no prompt character. Type a line and press Enter and
`sli3` evaluates it.

```
1 1 add =
2
```

That first line pushes `1`, then `1`, then runs `add` which pops two
values and pushes their sum, then runs `=` which pops the top of stack
and prints it followed by a newline. The result `2` is the printed value.

## Leaving

`quit` ends the interpreter:

```
quit
```

You can also send EOF (Ctrl-D on Linux/macOS) to the same effect.

## Running a file

Instead of typing at the prompt, you can put your program in a file and
pass it as an argument:

```
$ cat hello.sli
(hello) =
quit

$ ./build/sli3 hello.sli
SLI 1.0.0 (C) 2004 The NEST Initiative
 (C) 2026 Marc-Oliver Gewaltig
hello
```

A string in SLI is written inside parentheses: `(hello)`. The first line
pushes that string; the second prints it. Notice that `=` prints just the
string contents without the parentheses. The chapter on printing covers
the difference between `=` and `==`.

The tutorial examples under [`examples/`](examples/) are small `.sli`
files like the one above. You can run any of them directly.

## What to expect

Each following chapter introduces a small group of operators, shows them
in use, and ends with a few exercises. By chapter 10 you will know enough
to write small SLI programs that read input, compute, and produce output.
The full operator set is documented in [`core_commands.md`](core_commands.md);
that table is your reference once you are past the tutorial.

---

Next: [chapter 01 — the stack](01_stack.md).

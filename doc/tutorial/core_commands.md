# Core SLI commands — beginner set

This is the curated set of operators the beginner tutorial covers. Every
operator a new user needs to read, write, and run small SLI programs is
here. The list deliberately stops short of advanced topics
(trie internals, access modes, error machinery, serialization, state
snapshots) — see the *Excluded* section at the end for what was left out
on purpose.

Source citations point at the **C++ registration site** (where the
operator name first becomes callable) or, for operators defined in SLI, at
the `.sli` definition. Many operators are also wired through a polymorphic
*trie* in `lib/sli/typeinit.sli`; that wiring is not cited per-row to keep
the table compact.

---

## Stack

| Name | Source | Summary |
|------|--------|---------|
| `dup` | `src/builtins/sli_stack.cpp:359` | Duplicate the top element. |
| `pop` | `src/builtins/sli_stack.cpp:356` | Discard the top element. |
| `exch` | `src/builtins/sli_stack.cpp:360` | Swap the top two elements. |
| `over` | `src/builtins/sli_stack.cpp:370` | Copy the second element to the top. |
| `count` | `src/builtins/sli_stack.cpp:364` | Push the current stack depth. |
| `clear` | `src/builtins/sli_stack.cpp:365` | Discard every element on the operand stack. |
| `index` | `src/builtins/sli_stack.cpp:361` | Copy the element `n` deep, where `n` is on top. |
| `roll` | `src/builtins/sli_stack.cpp:363` | Cyclically rotate the top `n` elements by `j` positions. |

## Arithmetic

| Name | Source | Summary |
|------|--------|---------|
| `add` | `src/builtins/sli_math.cpp:1905` | Sum the top two numbers. |
| `sub` | `src/builtins/sli_math.cpp:1906` | Subtract the top number from the one below. |
| `mul` | `src/builtins/sli_math.cpp:1907` | Multiply the top two numbers. |
| `div` | `src/builtins/sli_math.cpp:1908` | Divide the one below by the top number. |
| `mod` | `src/builtins/sli_math.cpp:1899` | Integer modulo. |
| `neg` | `lib/sli/typeinit.sli:703` (trie over `neg_i`/`neg_d`) | Negate the top number. |
| `abs` | `lib/sli/typeinit.sli:680` (trie over `abs_i`/`abs_d`) | Absolute value of the top number. |
| `sqrt` | `src/builtins/sli_math.cpp:1924` | Square root. |
| `pow` | `src/builtins/sli_math.cpp:1925` | Power: `base exp pow`. |

## Comparison and boolean

| Name | Source | Summary |
|------|--------|---------|
| `eq` | `src/builtins/sli_math.cpp:1956` | Equal — works for numbers, names, strings. |
| `neq` | `src/builtins/sli_math.cpp:1977` | Not equal. |
| `gt` | `src/builtins/sli_math.cpp:1911` | Greater than. |
| `lt` | `src/builtins/sli_math.cpp:1912` | Less than. |
| `geq` | `src/builtins/sli_math.cpp:1913` | Greater than or equal. |
| `leq` | `src/builtins/sli_math.cpp:1914` | Less than or equal. |
| `and` | (trie on `booltype`/`integertype`) | Logical or bitwise AND. |
| `or` | (trie on `booltype`/`integertype`) | Logical or bitwise OR. |
| `not` | (trie on `booltype`/`integertype`) | Logical or bitwise NOT. |

## Control flow

| Name | Source | Summary |
|------|--------|---------|
| `if` | `src/builtins/sli_control.cpp:2015` | Run a procedure if a flag is true. |
| `ifelse` | `src/builtins/sli_control.cpp:2016` | Two-way branch on a flag. |
| `for` | `src/builtins/sli_control.cpp:2034` | Counted loop `start step stop { ... } for`. |
| `repeat` | `src/builtins/sli_control.cpp:2017` | Run a procedure `n` times. |
| `loop` | `src/builtins/sli_control.cpp:2013` | Infinite loop; terminate with `exit`. |
| `exit` | `src/builtins/sli_control.cpp:2014` | Break out of the innermost loop. |
| `forall` | `src/builtins/sli_control.cpp:2038` | Iterate a procedure over each array element. |
| `forallindexed` | `src/builtins/sli_control.cpp:2043` | `forall` that also pushes the index. |
| `exec` | `src/builtins/sli_control.cpp:2049` | Execute a procedure object on the operand stack. |

## Definitions and lookup

| Name | Source | Summary |
|------|--------|---------|
| `def` | `src/builtins/sli_control.cpp:2029` | Bind a name in the current dictionary. |
| `load` | `src/builtins/sli_control.cpp:2032` | Look up a name and push its value. |
| `lookup` | `src/builtins/sli_control.cpp:2033` | Look up a name; push value + true on success, false on miss. |
| `cvx` | `src/builtins/sli_typecheck.cpp:595` | Make an object executable (literal procedure → procedure). |
| `cvlit` | `src/builtins/sli_typecheck.cpp:598` | Make an object literal (procedure → literal procedure, name → literal name). |

## Arrays

| Name | Source | Summary |
|------|--------|---------|
| `[` `]` | (parser-level) | Array literal: every token between the brackets is collected into an array. |
| `length` | `src/builtins/sli_container_ops.cpp:1556` | Number of elements in an array or string. |
| `get` | `src/builtins/sli_container_ops.cpp:1557` | Element at a given index. |
| `put` | `src/builtins/sli_container_ops.cpp:1558` | Replace an element at a given index. |
| `append` | `src/builtins/sli_container_ops.cpp:1565` | Append an element; returns the new array. |
| `prepend` | `src/builtins/sli_container_ops.cpp:1566` | Prepend an element; returns the new array. |
| `join` | `src/builtins/sli_container_ops.cpp:1551` | Concatenate two arrays (or two strings). |
| `First` | `lib/sli/sli-init.sli:1167` (C++: `sli_array_module.cpp:801`) | First element of an array. |
| `Rest` | `lib/sli/sli-init.sli:1203` (C++: `sli_array_module.cpp:802`) | Array without its first element. |
| `array` | `lib/sli/sli-init.sli:858` (C++: `sli_array_module.cpp:800`) | New array of `n` zeros. |
| `Range` | `src/builtins/sli_array_module.cpp:799` (trie at `lib/sli/mathematica.sli:969`) | `[1..n]`, `[a..b]`, `[a step b]` — Mathematica-style range. |

## Dictionaries

| Name | Source | Summary |
|------|--------|---------|
| `<<` `>>` | `src/builtins/sli_control.cpp:2002`, `src/interpreter/sli_interpreter.cpp:381` | Dictionary literal: `<< /a 1 /b 2 >>`. |
| `dict` | `src/builtins/sli_startup.cpp:593` | Create an empty dictionary. |
| `begin` | `src/builtins/sli_startup.cpp:591` | Push a dictionary onto the dictstack so its keys resolve. |
| `end` | `src/builtins/sli_startup.cpp:592` | Pop the top dictionary off the dictstack. |
| `known` | `src/builtins/sli_container_ops.cpp` (registered via trie) | Test whether a key is in a dictionary. |
| `keys` | `src/builtins/sli_container_ops.cpp:1569` | Array of all keys in a dictionary. |

## Types and conversion

| Name | Source | Summary |
|------|--------|---------|
| `type` | `src/builtins/sli_typecheck.cpp:585` | Push the type-name literal of the top object. |
| `cvi` | `src/builtins/sli_typecheck.cpp:596` | Convert to integer. |
| `cvd` | `src/builtins/sli_typecheck.cpp:597` | Convert to double. |
| `cvs` | (trie wrapper around C++ converters) | Convert to string. |

## Printing and stack inspection

| Name | Source | Summary |
|------|--------|---------|
| `=` | `lib/sli/sli-init.sli:294` | Print the top of the stack and pop it. |
| `==` | `lib/sli/sli-init.sli:322` | Print the top of the stack in syntax form (with quotes and brackets) and pop it. |
| `=only` | `lib/sli/sli-init.sli:344` | Like `=`, no trailing newline. |
| `stack` | `lib/sli/sli-init.sli:391` | Print the whole operand stack without popping. |
| `pstack` | `lib/sli/sli-init.sli:447` | Like `stack`, but each entry in syntax form. |
| `endl` | `src/builtins/sli_startup.cpp:589` | Write a newline to a stream. |
| `<-` | `src/builtins/sli_startup.cpp:587` | Write an object to a stream. |
| `<--` | `src/builtins/sli_startup.cpp:588` | Write an object to a stream in syntax form. |
| `cout` | (preinstalled object in `systemdict`) | Standard output stream. |

## Utility

| Name | Source | Summary |
|------|--------|---------|
| `tic` | `lib/sli/sli-init.sli:563` | Start a wall-clock timer. |
| `toc` | `lib/sli/sli-init.sli:581` | Read elapsed seconds since the last `tic`. |
| `quit` | (built-in) | Leave the interpreter. |

---

## Excluded — defer to a later, intermediate tutorial

These operators are part of SLI but are not needed for a first pass:

- **Trie internals**: `trie`, `addtotrie`, `cva_t`, `cvt_a`, `trieinfo_os_t`.
- **Access modes**: `readonly`, `executeonly`, `noaccess`, `rcheck`, `wcheck`, `xcheck`.
- **Introspection**: `currentname`, `currentdict`, `dictstack`, `countdictstack`, `cleardictstack`, `execstack`, `operandstack`, `restoreestack`, `restoreostack`.
- **Debugging dumps**: `estackdump`, `ostackdump`, `backtrace_on`, `backtrace_off`.
- **Error machinery**: `stopped`, `stop`, `raiseerror`, `raiseagain`, `print_error`.
- **State snapshots**: `savestate`, `restorestate`, `restoredstack`.
- **Performance / timing internals**: `pclocks`, `pclockspersec`, `pgetrusage`, `ptimes`, `realtime`, `time`.
- **Advanced control**: `switch`, `switchdefault`, `case`, `bind`.
- **Regex**: `regcomp`, `regexec`, `regsplit` and `regexdict`.
- **Files**: `ifstream`, `ofstream`, `close`, `getline_is`, `eof`, `cvx_f`, `ostrstream`, `str` — covered only briefly, not as a chapter.
- **Internal helpers**: `bind_`, `def_`, `forall_a`, `forall_s`, `length_a`, `get_a` … the typed leaves below polymorphic operators. A beginner uses `add`, not `add_ii`.

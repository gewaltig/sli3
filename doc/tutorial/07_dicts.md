# 07. Dictionaries

A dictionary maps literal names (keys) to values. SLI uses dictionaries
both as a general-purpose key-value container and as the foundation for
name lookup — every name resolution walks a stack of dictionaries until
one of them has the key.

## Dictionary literals

Surround key-value pairs with `<<` and `>>`:

```
<< /a 1 /b 2 >> ==
<< /a 1 /b 2 >>
```

The parser reads `<<`, collects token pairs until `>>`, and builds a
dictionary. Keys must be literal names. Values can be any type,
including nested dictionaries and procedures.

```
<< /name (Alice) /age 30 /greet { (hi) = } >> ==
<< /name Alice /age 30 /greet <proceduretype> >>
```

`==` on a dictionary prints each value's printed form rather than its
literal syntactic form, so the string `(Alice)` shows as `Alice` and
the nested procedure shows as `<proceduretype>`. The dictionary's
actual contents are unchanged.

## Constructing programmatically

`dict` `(n -- dict)` creates an empty dictionary. The integer is a
capacity hint and is currently ignored; pass any non-negative value. To
add entries, push the dictionary onto the dictstack with `begin` and
use `def`:

```
10 dict dup begin
  /a 1 def
  /b 2 def
end ==
<< /a 1 /b 2 >>
```

`begin` pushes the dictionary onto the dictstack so that `def` finds it
as the current scope; `end` pops it. The final `==` prints the
dictionary still left on the operand stack (we kept it there with
`dup`).

In practice, the `<<` / `>>` literal is shorter and clearer; use the
programmatic form only when the entries are determined at runtime.

## Looking up entries

`get` `(dict /key -- value)` returns the value bound to the key.

```
<< /a 1 /b 2 >> /a get =
1
```

`known` `(dict /key -- bool)` tests whether a key is present.

```
<< /a 1 >> /a known =
true

<< /a 1 >> /c known =
false
```

## Modifying entries

`put` `(dict /key value -- )` adds or replaces an entry. Like array
`put`, this modifies the dictionary in place — there is no new dict on
the stack.

```
<< /a 1 >> dup /b 2 put ==
<< /a 1 /b 2 >>
```

`dup` keeps a reference to the dict, `put` adds the new entry, and the
final `==` prints the mutated dict.

## Keys and values

`keys` `(dict -- arr)` returns an array of all keys (literal names).

```
<< /a 1 /b 2 /c 3 >> keys ==
[/a /b /c]
```

The order of keys is implementation-defined. Don't rely on it.

## `begin` and `end`

`begin` `(dict -- )` pushes a dictionary onto the **dictstack**, making
its entries findable by bare-name lookup. `end` `( -- )` pops the top
dictionary off the dictstack.

```
<< /pi 3.14159 /e 2.71828 >> begin
  pi =
  e =
end
3.14159
2.71828
```

Inside the `begin`/`end` block, `pi` and `e` resolve from the literal
dictionary. After `end`, they no longer resolve unless they are also
defined in another dictionary on the stack.

## The dictstack at startup

When `sli3` starts, the dictstack has three entries (top to bottom):

1. **userdict** — your scratch space. Every `/foo X def` you type at the
   prompt goes here.
2. **globaldict** — for state that should outlive a `userdict` reset.
3. **systemdict** — the dictionary holding every built-in operator. After
   `sli-init.sli` finishes, `systemdict` is sealed and cannot be
   modified.

When you write an executable name, the interpreter looks it up from the
top dictionary downward. The first match wins. Pushing your own
dictionary onto the stack with `begin` is a way to introduce a small,
temporary scope that shadows the dictionaries below.

## A small example: keyword arguments

A procedure can take its arguments by name through a dictionary:

```
/greet
{
  begin
    (Hello, ) =only
    name =only
    (!) =
  end
} def

<< /name (Alice) >> greet
Hello, Alice!
```

`greet` expects a dictionary on the stack. Inside the body, `begin`
pushes that dictionary, then `name` resolves to `Alice` by name
lookup. `end` pops it again, leaving the dictstack as it was. `=only`
prints without a trailing newline; the final `=` adds the newline.

This pattern is widely used in NEST-era SLI for functions that take
many named options.

## Examples

- [`examples/ch07_dicts/literal.sli`](examples/ch07_dicts/literal.sli)
- [`examples/ch07_dicts/known_keys.sli`](examples/ch07_dicts/known_keys.sli)
- [`examples/ch07_dicts/greet.sli`](examples/ch07_dicts/greet.sli)

---

Next: [chapter 08 — types and conversion](08_types.md).

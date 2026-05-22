# 03. Names and procedures

So far every program has been a string of values and operators with
nothing reused. This chapter introduces **names** — SLI's equivalent of
variables — and **procedures** — its equivalent of functions. Once you
can bind a name, you can give a value or a piece of code a label and
look it up later.

## Names

A bare identifier in SLI is executable: when the interpreter reads it,
it looks the name up and runs the value bound to it. `add` is a name
bound to the addition operator. `dup` is a name bound to the
duplication operator.

If you want to refer to a name without running it, prefix it with a
slash. `/foo` is a **literal name** — it is pushed onto the stack as a
value instead of being looked up.

```
/foo type =
/literaltype

/add load type =
/functiontype
```

The first prints the type of the literal name `/foo`. The second uses
`load` to look up the value bound to `/add` and pushes it onto the
stack; `type` reports `/functiontype` because `add` is a built-in
implemented in C++.

If you write `add` without the slash, the interpreter tries to run it
immediately. With an empty stack you would get a `StackUnderflow` error
because `add` expects two operands.

## Binding a name with `def`

`def` `(name value -- )` takes a literal name and a value and binds them
in the current dictionary.

```
/pi 3.141593 def
pi =
3.14159
```

Once defined, `pi` behaves exactly like a literal. You can use it in any
expression where a number is expected:

```
2 pi mul =
6.28319
```

`def` works with values of any type. The next section binds a piece of
SLI code.

## Procedures

A procedure is a piece of SLI code wrapped in braces:

```
{ 1 add }
```

By itself the brace pair does nothing visible — it pushes a procedure
object onto the stack without running it. To run it, use `exec`:

```
3 { 1 add } exec =
4
```

Run inside the braces: `3` is already on the stack, the procedure
pushes `1`, then `add` consumes both and pushes `4`.

To make procedures useful, bind them to a name:

```
/inc { 1 add } def
5 inc =
6
```

Now `inc` is callable like a built-in operator. Names bound to
procedures execute the procedure body when used (no explicit `exec`
needed).

Here is `hypot` from the previous chapter as a procedure:

```
/hypot { dup mul exch dup mul add sqrt } def
3 4 hypot =
5
```

`hypot` expects two numbers on the stack and leaves the hypotenuse.

## The dictionary stack

Names are stored in **dictionaries**. There is a stack of dictionaries
called the dictionary stack. When you write a bare name, the
interpreter walks the dictstack from top to bottom and uses the first
dictionary that has that name as a key.

After startup the dictstack has three entries:

- `userdict` — top. Every `def` you write is stored here.
- `globaldict` — for state that should outlive a `userdict` reset.
- `systemdict` — bottom. Holds every built-in operator (`add`, `sqrt`,
  `def` itself). After startup `systemdict` is sealed and cannot be
  modified.

You normally do not have to think about which dictionary holds a name.
The chapter on dictionaries (chapter 07) covers manipulating the
dictstack explicitly.

## `load` and `lookup`

There are two more name-related operators that come in handy:

`load` `(/name -- value)` looks the name up and pushes its value. It is
what an executable name does internally, exposed for explicit use.

```
/x 42 def
/x load =
42
```

`lookup` `(/name -- value true | false)` is a "try" version of `load`.
On success it pushes the value and `true`. On miss it pushes only
`false`.

```
/notthere lookup =
false
```

You will see `lookup` in code that needs to check whether a key is bound
before using it.

## Literal vs executable

Earlier we used the term **executable name** for `add` and **literal
name** for `/add`. The distinction generalises:

- An **executable** object runs when reached on the execution stack.
- A **literal** object is pushed unchanged onto the operand stack.

The main place this comes up is between **arrays** and **procedures**.
A procedure (braces) is executable: when stored anywhere and reached,
it runs. An array (square brackets) is literal: it sits on the stack as
a value.

`cvx` converts an array of tokens into a procedure that, when reached,
executes those tokens. `cvlit` does the reverse for a procedure:

```
{ 1 add } type =
/proceduretype

{ 1 add } cvlit type =
/arraytype
```

After `cvlit` the same body is an array of tokens; the storage is
identical (a procedure IS an array under the hood) but the type tag
controls whether it runs on use.

`cvx` is mostly used to take an array you have built or read from disk
and turn it into runnable code:

```
[ 1 2 add ] cvx exec =
3
```

You will rarely need `cvx` or `cvlit` in everyday SLI; chapter 08 fills
in the rest of the conversion table.

## Naming style and shadowing

Names in SLI are case-sensitive. There is no separator between letters
and digits inside a name; `foo123bar` is one name. You can shadow any
name by binding it in a higher dictionary — `/add { 99 } def` overrides
`add` in `userdict` and you lose access to the original until you
remove your version. Don't do this unless you mean to.

## Examples

- [`examples/ch03_names/def.sli`](examples/ch03_names/def.sli)
- [`examples/ch03_names/hypot.sli`](examples/ch03_names/hypot.sli)
- [`examples/ch03_names/lookup.sli`](examples/ch03_names/lookup.sli)

---

Next: [chapter 04 — conditionals](04_conditionals.md).

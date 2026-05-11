# Ghostscript 10.07 interpreter — architecture reference

Reference notes on how Ghostscript implements the PostScript
interpreter. Written for sli3 contributors deciding where to
borrow or diverge. Cross-references the upstream source at
`/tmp/ghostpdl-10.07/` (clone from
`https://github.com/ArtifexSoftware/ghostpdl`, tag
`ghostpdl-10.07.0`).

Coverage:

- Data model — `ref`, `ref_packed`, type encoding
- The three stacks — operand, execution, dictionary
- The interpreter loop topology (`interp()`)
- Procedure execution and proc-body iteration
- Operator dispatch — hot ops inlined, generic ops via call
- Loop operators (`for` / `repeat` / `loop` / `forall`)
- `bind` operator
- Name lookup with `pvalue` cache
- Error path and stack snapshots in `$error`
- What `signalerror` / `handleerror` actually do

All paths below are relative to `/tmp/ghostpdl-10.07/` unless
otherwise noted.

---

## 1. The `ref` data model

### 1.1 Layout

A full ref is 16 bytes on a 64-bit build. Definition at
`psi/iref.h:427-457`:

```c
struct tas_s {
    ushort   type_attrs;   /* type id (top byte) + attribute bits */
    ushort   _pad;
    uint32_t rsize;        /* type-dependent size / index field */
};
struct ref_s {
    struct tas_s tas;       /* 8 bytes */
    union ref_value {       /* 8 bytes */
        ps_int            intval;    /* t_integer */
        ushort            boolval;   /* t_boolean */
        float             realval;   /* t_real */
        byte             *bytes;     /* t_string */
        ref              *refs;      /* t_array */
        name             *pname;     /* t_name */
        dict             *pdict;     /* t_dictionary */
        const ref_packed *packed;    /* t_mixedarray / t_shortarray */
        op_proc_t         opproc;    /* t_operator */
        struct stream_s  *pfile;     /* t_file */
        obj_header_t     *pstruct;   /* t_struct / t_astruct / t_fontID */
        ...
    } value;
};
```

`type_attrs` is a 16-bit field packing the type id (bits 8-13)
and attribute bits (bits 0-7). It is deliberately the first
member so the same byte position can be used to distinguish
full refs from packed refs (see §1.2). Macros that read the
type-attrs byte:

- `r_type_xe(rp)` — type + executable + execute-attr bits
  (`iref.h:493+`). Used as the dispatcher switch key.
- `r_type(rp)` — type id only.
- `r_btype(rp)` — type id, normalised for the extended pseudo-
  types used for hot operators (`tx_op_*` collapse to
  `t_operator`).
- `r_has_attr(rp, a_*)` — attribute test.

### 1.2 Attributes

Attribute bits (`iref.h:331-359`):

| bit | name | meaning |
|---|---|---|
| 0x01 | `l_mark` | GC mark (location attr, not ref attr) |
| 0x02 | `l_new`  | dirty since last save |
| 0x04, 0x08 | `r_space_*` | VM space (local / global / system) |
| 0x10 | `a_write` | write permission |
| 0x20 | `a_read`  | read permission |
| 0x40 | `a_execute` | execute permission |
| 0x80 | `a_executable` | the ref is to be executed (vs literal) |

`a_executable` controls procedure vs literal behaviour. The
parser sets it for executable arrays (`{ ... }`) and bare
operator names; leaves it clear for literals (`/foo`, `[...]`).

### 1.3 Type IDs

Type ids occupy 6 bits, room for 64 types. The base PostScript
types are at `iref.h:124-208`:

```
t__invalid, t_boolean, t_dictionary, t_file,
t_array, t_mixedarray, t_shortarray, t_unused_array_,
t_struct, t_astruct,
t_integer, t_real, t_fontID, t_mark, t_name, t_null,
t_operator, t_save, t_string,
t_device, t_oparray, t_pdfctx,
t_next_index                       /* extension space starts here */
```

Important properties baked into the numbering:

- The four array types (`t_array`, `t_mixedarray`,
  `t_shortarray`, `t_unused_array_`) are consecutive and start
  on a multiple of 4 so `r_is_array()` and `r_is_proc()` are
  range tests (`iref.h:131-143`).
- `t_integer` and `t_real` are consecutive starting on a multiple
  of 2 so `r_is_number()` is a one-test predicate
  (`iref.h:171-176`).
- Type ids from `t_next_index` upward are extended pseudo-types
  used for *hard-coded hot operators* (`tx_op_add`, `tx_op_pop`,
  …). When a ref of one of these types appears, it really is
  still a `t_operator`; only the dispatcher distinguishes them
  for fast dispatch. `r_btype` collapses them back to
  `t_operator` so non-dispatcher code sees the regular type
  (`iref.h:213-217`, `interp.c:122`).

### 1.4 Packed refs (the "compact" form)

A `ref_packed` is a `ushort` — 2 bytes. Definition and encoding
at `psi/ipacked.h:24-79`:

```
00tttttt exrwsfnm    full-size ref (type bits 0-31)
010mjjjj jjjjjjjj    executable operator (12-bit op index)
011mvvvv vvvvvvvv    integer (biased by -2048; range -2048..+2047)
110miiii iiiiiiii    literal name (12-bit name index)
111miiii iiiiiiii    executable name (12-bit name index)
```

The top 2 bits distinguish packed from full refs: any value
`>= 0x4000` (i.e. with one of bits 14-15 set) is a packed
slot; otherwise it's the `tas.type_attrs` of a full ref. The
test is `r_is_packed(rp) = (*(ref_packed*)rp >= pt_tag(pt_min_packed))`
(`ipacked.h:104`).

Two array kinds use packed refs:

- `t_shortarray` — all slots are packed.
- `t_mixedarray` — some slots packed, some full refs;
  preceding packed slots are converted to full refs as
  needed to keep alignment for the full refs
  (`ipacked.h:65-75`).

The dispatcher's iteration macros (`interp.c:1066-1083`) cope
with both:

```c
#define IREF_NEXT(ip)       ((const ref_packed *)((const ref *)(ip) + 1))
#define IREF_NEXT_EITHER(ip) (r_is_packed(ip) ? (ip)+1 : IREF_NEXT(ip))
#define next_either()\
  if (--icount <= 0) { if (icount < 0) goto up; iesp--; }\
  iref_packed = IREF_NEXT_EITHER(iref_packed); goto top
```

Operator names that don't fit in 12 bits and integers that
don't fit in [-2048, 2047] are stored as full 16-byte refs
inside a `t_mixedarray`. Names with permanent dictionary
binding are interned with low indices so they fit; most
hot operators have indices that fit.

---

## 2. The three stacks

All three stacks (operand, execution, dictionary) share an
implementation: linked blocks of refs, growable.

### 2.1 Block-list stack data structure

`psi/isdata.h:67-87`:

```c
struct ref_stack_s {
    s_ptr   p;          /* current top element */
    s_ptr   bot;        /* bottommost valid element in current block */
    s_ptr   top;        /* topmost valid element in current block */
    ref     current;    /* t_array for current top block */
    uint    extension_size;
    uint    extension_used;
    ref     max_stack;
    uint    requested;
    uint    margin;
    uint    body_size;
    ...
};
```

A stack is a chain of `ref_stack_block` segments
(`istack.h:51-55`):

```c
typedef struct ref_stack_block_s {
    ref next;   /* t_array, next lower block on stack */
    ref used;   /* t_array, subinterval of this block */
    /* Then the actual refs follow in memory. */
} ref_stack_block;
```

Each block is itself a PostScript array so the GC can scan it.
The top block is hot; deeper blocks are cold. `bot` and `top`
are the bounds of the current top block; `p` is the live top.

### 2.2 Stack-pointer macros

- **Operand stack** (`psi/ostack.h:25-71`): `osp` (top), `osbot`,
  `ostop`, with macros `push(n)`, `pop(n)`, `check_op(nargs)`,
  `check_ostack(n)`.
- **Execution stack** (`psi/estack.h`): `esp`, `esbot`, `estop`,
  `check_estack(n)`, plus mark/cleanup helpers
  `push_mark_estack(es_type, cleanup)`,
  `make_mark_estack(p, es_type, cleanup)`,
  `make_op_estack(p, opproc)`.
- **Dictionary stack** (`psi/dstack.h`): `dsp` ... etc.

Inside `interp()` the hot path keeps **private register copies**
of `osp` and `esp` (`iosp` and `iesp`, `interp.c:979-980`) and
syncs back to the shared `osp` / `esp` only when calling out to
operator C functions. This is critical: the inner dispatch loop
never touches memory for the stack pointers.

### 2.3 What lives on each stack

- **Operand stack** — values pushed by operators and by literal
  evaluation. PostScript user code reads it directly.
- **Execution stack** — call frames: procedures being iterated,
  marks for `stopped` / `for` / oparray cleanup, and
  continuation operators (`for_pos_int_continue`,
  `repeat_continue`, `loop_continue`, `oparray_pop`). It does
  *not* contain individual hot operator refs during their
  execution — the dispatcher tracks the current ref via the
  register-held `iref_packed` (see §3). It does contain
  procedure refs being interpreted: when entering a procedure
  body, the proc ref is pushed onto the e-stack and the
  dispatcher iterates over its contents.
- **Dictionary stack** — dicts that participate in name lookup;
  `begin` / `end` push and pop.

---

## 3. The interpreter loop topology

`gs_interpret()` (`psi/interp.c:464`) is the top-level entry
called from outside. It calls `interp()` (`:947`) which is the
hot loop.

### 3.1 `interp()` register state

`psi/interp.c:947-1100`:

```c
interp(i_ctx_t **pi_ctx_p, const ref *pref, ref *perror_object) {
    register const ref_packed *iref_packed = (const ref_packed *)pref;
    register int   icount = 0;
    register os_ptr iosp = osp;
    register es_ptr iesp = esp;
    int code;
    ref token;
    ref *pvalue;
    ref refnull;
    uint opindex;
    os_ptr whichp;
    ...
    if (iesp >= estop) return_with_error(...);
    ++iesp;
    ref_assign_inline(iesp, pref);
    goto bot;
}
```

The function uses local `iosp` / `iesp` so the compiler keeps
them in registers. They are written back to the global `osp` /
`esp` (`i_ctx_p->op_stack.stack.p` and
`i_ctx_p->e_stack.stack.p`) only at points where a C operator
function is called — operators read/write `osp` directly. After
the operator returns, `iosp = osp; iesp = esp;` re-syncs.

### 3.2 The `top:` label

`psi/interp.c:1100-1156`:

```c
top:
    INCR(top);
    /* debug-only validation snipped */
    switch (r_type_xe(iref_packed)) {
        cases_invalid():  return_with_error_iref(gs_error_Fatal);
        cases_nox():      return_with_error_iref(gs_error_invalidaccess);
        cases_lit_1():
        cases_lit_2():
        cases_lit_3():
        cases_lit_4():
        cases_lit_5():
            INCR(lit);
            break;       /* falls through to bot: which does next() */
        cases_lit_array():
            INCR(lit_array);
            break;
        case plain_exec(tx_op_add):  ... next_either();
        case plain_exec(tx_op_def):  ... next_either();
        case plain_exec(tx_op_dup):  ... next_either();
        case plain_exec(tx_op_exch): ... next_either();
        case plain_exec(tx_op_if):   ... goto ifup;
        case plain_exec(tx_op_ifelse): ... goto ifup;
        case plain_exec(tx_op_index): ... next_either();
        case plain_exec(tx_op_pop):  ... next_either();
        case plain_exec(tx_op_roll): ... next_either();
        case plain_exec(tx_op_sub):  ... next_either();
        case plain_exec(t_null):     goto bot;
        case plain_exec(t_oparray):  ... goto opst;
        case plain_exec(t_operator): ...
        case plain_exec(t_name):     ...
        case exec(t_file):           ...
        case exec(t_string):         ...
        case exec(t_array):          ... goto prst;
        case exec(t_mixedarray):     ... goto prst;
        case exec(t_shortarray):     ... goto prst;
    }
```

The switch is the entire interpreter. Each arm either advances
`iref_packed` (`next()` / `next_either()` / `goto bot`) and
returns to `top:`, or transitions to a different state via
`goto up:` / `goto slice:` / `goto opush:` / `goto opop:`.

`cases_lit_*()` (`interp.c:1174-1198`) groups all literal types
into one shared arm that pushes the literal onto the operand
stack at `bot:` and advances.

### 3.3 The advance macros

`psi/interp.c:1076-1083`:

```c
#define next()\
  if (--icount > 0) { iref_packed = IREF_NEXT(iref_packed); goto top; }\
  else goto out
#define next_short()\
  if (--icount <= 0) { if (icount < 0) goto up; iesp--; }\
  ++iref_packed; goto top
#define next_either()\
  if (--icount <= 0) { if (icount < 0) goto up; iesp--; }\
  iref_packed = IREF_NEXT_EITHER(iref_packed); goto top
```

`icount` is the number of refs *remaining after* the current
one in the active body. When it reaches 0, the body is done
and control falls to `out:` (which checks for tail-recursion)
or `up:` (which advances to the next outer body).

### 3.4 The `bot:` `out:` `up:` triad

`psi/interp.c:1853-1882`:

```c
bot: next();
out:                /* exactly 0 left (we already decremented) */
    if (!icount) {
        iesp--;             /* pop the just-finished body frame */
        iref_packed = IREF_NEXT(iref_packed);
        goto top;           /* tail-recurse into the parent body */
    }
up: if (--(*ticks_left) < 0) goto slice;
    if (!r_is_proc(iesp)) {  /* a continuation op or other non-proc */
        SET_IREF(iesp--);
        icount = 0;
        goto top;
    }
    SET_IREF(iesp->value.refs);     /* enter next outer proc body */
    icount = r_size(iesp) - 1;
    if (icount <= 0) {
        iesp--;
        if (icount < 0) goto up;
    }
    goto top;
```

`bot:` is the "after a literal" path. `out:` and `up:` together
implement tail recursion: when a procedure finishes, the
dispatcher doesn't push a "return from proc" frame — it pops
the proc frame and continues directly in the outer body.

If the new e-stack top is *not* a proc (e.g. it's a continuation
operator like `for_pos_int_continue`), the dispatcher executes
that op via `SET_IREF(iesp--); icount = 0; goto top;` — the op
ref becomes the new `IREF` and gets dispatched at `top:`. The
continuation op typically returns `o_push_estack` (push a fresh
copy of the proc) or `o_pop_estack` (we're done).

---

## 4. Procedure execution

PostScript procedures are arrays with `a_executable` set.
Entering a procedure pushes its ref onto the e-stack and starts
walking its contents.

### 4.1 Entering a procedure

When a `t_array` / `t_mixedarray` / `t_shortarray` with
`a_executable` set is encountered, the dispatcher jumps to
`prst:` / `pr:` (`psi/interp.c:1339-1361`):

```c
prst: store_state(iesp);
pr:   if (iesp >= estop) return_with_error_iref(gs_error_execstackoverflow);
      if ((icount = r_size(pvalue) - 1) <= 0) {
          if (icount < 0) goto up;             /* 0-element proc */
          SET_IREF(pvalue->value.refs);        /* 1-element proc */
          if (--(*ticks_left) > 0) goto top;
      }
      ++iesp;
      iesp->tas = pvalue->tas;
      SET_IREF(iesp->value.refs = pvalue->value.refs);
      if (--(*ticks_left) > 0) goto top;
      goto slice;
```

`store_state(iesp)` (`interp.c:1070-1075`):

```c
#define store_state(ep)\
  ( icount > 0 ? (ep->value.const_refs = IREF + 1, r_set_size(ep, icount)) : 0 )
```

This saves the *outer* body's remaining position (just past the
current ref) and count into the outer frame on the e-stack
before entering the inner proc. When the inner proc returns
(via `up:`), the outer frame's `value.refs` becomes the new
`iref_packed` and `r_size` becomes the new `icount`.

### 4.2 Tail-recursion

`out:` (`interp.c:1859-1866`) pops the *current* frame when
icount has just gone to 0 — no re-entry to the switch. The
dispatcher slides directly into the outer body via
`iref_packed = IREF_NEXT(iref_packed); goto top;`. This is the
key optimization that gives gs cheap procedure calls.

### 4.3 0-element and 1-element procedures

Both `prst:` and the hot operator dispatch (`opst`, `ifup`) have
fast paths: a 1-element proc skips the e-stack push entirely
and just sets `iref_packed` to the single element; a 0-element
proc jumps to `up:` to pop back. This avoids `iesp` traffic for
trivial procs (common after some patterns of `bind`).

---

## 5. Operator dispatch

### 5.1 Hot operators inlined

`psi/interp.c:1213-1317` contains the inlined bodies of ten
operators. Example for `add` (`:1213-1219`):

```c
case plain_exec(tx_op_add):
x_add: INCR(x_add);
    osp = iosp;
    if ((code = zop_add(i_ctx_p)) < 0) return_with_error_tx_op(code);
    iosp--;
    next_either();
```

`pop` is shorter (`:1298-1303`):

```c
case plain_exec(tx_op_pop):
x_pop: INCR(x_pop);
    if (iosp < osbot) return_with_error_tx_op(gs_error_stackunderflow);
    iosp--;
    next_either();
```

The full inlined set: `add`, `def`, `dup`, `exch`, `if`,
`ifelse`, `index`, `pop`, `roll`, `sub`. Each ends with
`next_either()` to advance the body iterator and return to
`top:`.

The numbers come from extending `t_next_index` with synthetic
type ids `tx_op_add`, `tx_op_pop`, etc. (`psi/interp.h` and
internal init code in `psi/iinit.c`). When `bind` or operator
registration creates a ref for one of these ops, it stamps the
hot type id into `tas.type_attrs`. Code outside the dispatcher
still sees `r_btype() == t_operator` (`iref.h:213-217`).

The `if` and `ifelse` arms are different — they enter a
procedure body inline rather than calling a C function
(`interp.c:1246-1291`). `x_if` pops the boolean and proc from
the operand stack, then jumps to `ifup:` which is shared with
`pr:` for the proc-entry sequence.

### 5.2 Generic operator dispatch

`psi/interp.c:1362-1420` — the `t_operator` arm for operators
that aren't in the hot set:

```c
case plain_exec(t_operator):
    INCR(exec_operator);
    if (--(*ticks_left) <= 0) { /* GC check */ }
    esp = iesp;                  /* sync */
    osp = iosp;
    switch (code = call_operator(real_opproc(IREF), i_ctx_p)) {
        case 0:                  /* normal success */
        case 1:                  /* alternative success */
            iosp = osp;
            next();
        case o_push_estack:
            store_state(iesp);
          opush: iosp = osp; iesp = esp;
            if (--(*ticks_left) > 0) goto up;
            goto slice;
        case o_pop_estack:
          opop: iosp = osp;
            if (esp == iesp) goto bot;
            iesp = esp;
            goto up;
        case gs_error_Remap_Color:
            goto oe_remap;
    }
    iosp = osp; iesp = esp;
    return_with_code_iref();      /* any other code is an error */
```

The operator function (`real_opproc(IREF)` — a function
pointer stored in `IREF->value.opproc`) receives `i_ctx_p` and
operates on `osp` directly. It returns:

| code | meaning |
|---|---|
| `0` or `1` | success; iosp updated; advance to next ref |
| `o_push_estack` | I pushed onto e-stack; please continue interpreting from the new top |
| `o_pop_estack` | done with my e-stack frame; pop it |
| `gs_error_Remap_Color` | special path |
| `< 0` | error code; raise |

Notice: **the operator's own ref is NOT pushed onto the e-stack
before the call**. The dispatcher already knows the operator's
identity via `IREF` (the register-held current ref).

### 5.3 Name dispatch

`psi/interp.c:1421-1522` — the `t_name` arm:

```c
case plain_exec(t_name):
    INCR(exec_name);
    pvalue = IREF->value.pname->pvalue;
    if (!pv_valid(pvalue)) {
        uint nidx = names_index(int_nt, IREF);
        uint htemp = 0;
        if ((pvalue = dict_find_name_by_index_inline(nidx, htemp)) == 0)
            return_with_error_iref(gs_error_undefined);
    }
    /* Dispatch on the type of the value. */
    switch (r_type_xe(pvalue)) {
        cases_lit_*():
            /* push the literal */
        case exec(t_array): case exec(t_mixedarray):
        case exec(t_shortarray):
            goto prst;                          /* enter the procedure */
        case plain_exec(tx_op_add): goto x_add; /* reuse the hot arm */
        case plain_exec(tx_op_def): goto x_def;
        ...
        case plain_exec(t_operator):
            ...                                  /* shortcut for ops */
        ...
    }
```

The dispatcher follows the name → value resolution and *jumps
to the same arm as if it had encountered the value directly*.
Hot operators reached via name dispatch share their inline
body via `goto x_add` etc. This means `1 1 add` and `/foo /add
load def 1 1 foo` both end up in the same code path.

### 5.4 Name lookup with `pvalue` cache

The key trick at line 1423 is `pvalue = IREF->value.pname->pvalue;`.
Every `name` struct (`psi/inamedef.h:58-68`) has a `pvalue`
field:

```c
struct name_s {
#define pv_no_defn ((ref *)0)
#define pv_other ((ref *)1)
#define pv_valid(pvalue) ((uintptr_t)(pvalue) > 1)
    ref *pvalue;        /* fast cache */
};
```

When a name is bound in only one dictionary (typically
`systemdict` or `userdict`), `pvalue` points directly at the
value ref. Lookup is **one pointer indirection** — no hash
walk. When the cache is invalid (`pv_no_defn` or `pv_other`,
meaning the name has multiple definitions in the dict stack
or isn't defined), the dispatcher falls back to
`dict_find_name_by_index_inline` which is an inlined hash walk
through the dict stack.

This is gs's analog to sli3's dictstack lookup, but cheaper:
the common case is a single load through the name's pvalue
slot.

---

## 6. Loop operators

### 6.1 `repeat`

`psi/zcontrol.c:464-498`:

```c
int zrepeat(i_ctx_t *i_ctx_p) {
    os_ptr op = osp;
    check_op(2);
    check_proc(*op);
    check_type(op[-1], t_integer);
    if (op[-1].value.intval < 0) return_error(gs_error_rangecheck);
    check_estack(5);
    push_mark_estack(es_for, no_cleanup);   /* mark frame */
    *++esp = op[-1];                         /* count */
    *++esp = *op;                            /* proc */
    make_op_estack(esp + 1, repeat_continue);
    pop(2);
    return repeat_continue(i_ctx_p);
}

static int repeat_continue(i_ctx_t *i_ctx_p) {
    es_ptr ep = esp;
    if (--(ep[-1].value.intval) >= 0) {   /* still iterations left */
        esp += 2;
        ref_assign(esp, ep);              /* push another copy of proc */
        return o_push_estack;
    } else {
        esp -= 3;                          /* pop mark, count, proc */
        return o_pop_estack;
    }
}
```

E-stack layout after `zrepeat` runs:

```
[..., mark(es_for), count, proc, repeat_continue]   <-- esp
```

The dispatcher returns to `opush:` → `up:` which sees
`repeat_continue` on top (not a proc, so it gets dispatched as
an operator). `repeat_continue` either:

- decrements `count`, pushes another copy of `proc` (now esp is
  `[..., mark, count, proc, repeat_continue, proc]`), returns
  `o_push_estack` → dispatcher enters that proc body; or
- pops the frame entirely and returns `o_pop_estack`.

Note `repeat_continue` itself sits permanently at the top of
the e-stack between iterations; the proc ref above it is what
the dispatcher walks.

### 6.2 `for`

`psi/zcontrol.c:288-405`. Three flavors selected by argument
types: `for_pos_int_continue`, `for_neg_int_continue`,
`for_real_continue`. The frame layout is similar to repeat
plus more state:

```
[..., mark(es_for), control_var, increment, limit, proc, for_*_continue]
```

The continuation operator checks the control variable against
the limit; if more iterations, pushes the variable onto the
operand stack (this is the integer that PostScript's `for`
makes available to the procedure body) and pushes another proc
copy.

### 6.3 `loop`

`psi/zcontrol.c:501-527`:

```c
static int zloop(i_ctx_t *i_ctx_p) {
    check_op(1);
    check_proc(*op);
    check_estack(4);
    push_mark_estack(es_for, no_cleanup);
    *++esp = *op;                             /* proc */
    make_op_estack(esp + 1, loop_continue);
    pop(1);
    return loop_continue(i_ctx_p);
}

static int loop_continue(i_ctx_t *i_ctx_p) {
    register es_ptr ep = esp;
    ref_assign(ep + 2, ep);
    esp = ep + 2;
    return o_push_estack;
}
```

Pure infinite repeat. `exit` is the way out; `zexit`
(`zcontrol.c:531-557`) walks the e-stack looking for an
`es_for` mark and pops everything above (and including) it.

### 6.4 `forall`

`forall` is implemented as a C continuation operator per
collection type, similar to `repeat`. See
`/tmp/ghostpdl-10.07/psi/zgeneric.c` for the array/dict/string
variants. The pattern is identical: set up an e-stack frame
with current-index state and a continuation op; on each call,
push the next element + a fresh copy of the proc.

---

## 7. `bind`

`psi/zmisc.c:46-166`. The operator walks a procedure body
once, in place, and rewrites every executable name slot whose
dictstack lookup yields an executable operator (or oparray) to
hold the operator ref directly. Names whose values are
procedures, integers, etc. are *not* rewritten — PostScript
semantics require those to stay late-bound.

Key logic (paraphrased from `zmisc.c:82-153`):

```c
while (depth) {
    while (r_size(bsp)) {          /* bsp points at the array currently being walked */
        ref_packed *tpp = (ref_packed *)bsp->value.packed;
        r_dec_size(bsp, 1);
        if (r_is_packed(tpp)) {
            if (r_packed_is_exec_name(&elt)) {
                name_index_ref(..., &nref);
                if ((pvalue = dict_find_name(&nref)) != 0 && r_is_ex_oper(pvalue)) {
                    /* rewrite packed slot to a packed operator ref */
                    *tpp = pt_tag(pt_executable_operator) + op_index(pvalue);
                }
            }
            bsp->value.packed = tpp + 1;
        } else {
            ref *tp = bsp->value.refs++;
            switch (r_type(tp)) {
            case t_name:
                if (r_has_attr(tp, a_executable)) {
                    if ((pvalue = dict_find_name(tp)) != 0 && r_is_ex_oper(pvalue)) {
                        ref_assign_old(bsp, tp, pvalue, "bind");
                    }
                }
                break;
            case t_array: case t_mixedarray: case t_shortarray:
                if (r_has_attr(tp, a_executable)) {
                    /* Recurse: push the inner array onto bsp's stack */
                    r_clear_attrs(tp, a_write);
                    *++bsp = *tp; depth++;
                }
            }
        }
    }
    bsp--; depth--;
}
```

`bind` uses the operand stack itself as a workstack: it pushes
nested procedure arrays onto `bsp` and increments `depth`,
descending recursively. When a sub-array finishes, `bsp--` and
`depth--` pop back to the parent.

Side-effects of `bind`:

- Inner procedures become read-only (`r_clear_attrs(tp, a_write)`).
- Executable-name slots that resolved to operators become
  operator slots.
- Names that don't resolve, or resolve to non-operators
  (numbers, procedures, dicts), stay as name slots.

PLRM3 special case: if the *top-level* procedure has had its
write attribute removed already, `bind` is a no-op
(`zmisc.c:57`).

The `is_ex_oper` test (`zmisc.c:40-45`) accepts both
`t_operator` and `t_oparray` so that user-defined operators
implemented as procedures get bound too.

---

## 8. Error path

### 8.1 Capturing the error in `interp()`

When something goes wrong inside the dispatcher, control jumps
to one of three labels (`psi/interp.c:1925-1942`):

```c
rweci:  ierror.code = code;
rwei:   ierror.obj = IREF;            /* <-- the current ref IS the failing object */
rwe:    if (!r_is_packed(iref_packed))
            store_state(iesp);         /* preserve outer body position */
        else {
            packed_get(imemory, (const ref_packed *)ierror.obj, &ierror.full);
            store_state_short(iesp);
            ...
        }
error_exit:
    ...
    ref_assign_inline(perror_object, ierror.obj);
    return gs_log_error(ierror.code, __FILE__, ierror.line);
```

The failing object is whatever `IREF` was pointing at when the
error fired. This is gs's analog to "current operator" — it's
the register state, captured at error time, returned via
`perror_object` to the caller.

`store_state(iesp)` writes the current position back into the
e-stack frame so a snapshot or traceback can see how far the
proc had run.

### 8.2 The outer error loop in `gs_interpret`

`psi/interp.c:464-762` is the wrapper around `interp()`. It
calls `interp()` in a `while`-like structure (using `goto
again:`):

```c
gs_interpret(...) {
    ...
again:
    ...
    code = interp(pi_ctx_p, epref, perror_object);
    ...
    switch (code) {
        case gs_error_Quit:           ...
        case gs_error_InterpreterExit: return 0;
        case gs_error_dictstackoverflow:    /* handle stack growth */ ...
        case gs_error_stackoverflow:        ...
        case gs_error_execstackoverflow:    ...
        ...
    }
    /* Otherwise: look up `error_name` in errordict and call it as a proc */
    if ((dict_find_string(systemdict, "gserrordict", &perrordict) <= 0
         || dict_find(perrordict, &error_name, &epref) <= 0)
        && (dict_find_string(systemdict, "errordict", &perrordict) <= 0
            || dict_find(perrordict, &error_name, &epref) <= 0))
        return code;
    doref = *epref;
    epref = &doref;
    osp++; *osp = *perror_object;   /* push the failing object on operand stack */
    errorexec_find(i_ctx_p, osp);
    /* normalise non-name/non-string objects to a name for error reporting */
    ...
    goto again;
}
```

The full sequence on error:

1. `interp()` returns with a negative code; `*perror_object`
   holds `IREF` at fault time.
2. `gs_interpret` resolves the error code to a name
   (`errorname`).
3. It looks up the error handler in `errordict` / `gserrordict`
   by that name.
4. It pushes the failing object onto the operand stack.
5. It calls `interp()` again with the handler proc as the new
   `pref`. Control returns to `again:`.

This is how `1 0 div` ends up invoking
`errordict/undefinedresult` with `div` on the operand stack:
the dispatcher captures `IREF = /div` at fault, the wrapper
pushes `/div` onto ostack and dispatches the handler.

### 8.3 `signalerror` and `.errorhandler`

The handlers themselves are SLI-level — defined in PostScript
in `Resource/Init/gs_init.ps:1158-1201`:

```ps
/.errorhandler            % <command> <errorname> .errorhandler -
{   1 .instopped { //null eq { pop pop stop } if } if
    //.acquire_$error exec /.inerror get 1 .instopped { pop } { pop //true } ifelse
        { //.unstoppederrorhandler exec } if
    //.acquire_$error exec /globalmode .currentglobal //false .setglobal put
    //.acquire_$error exec /.inerror //true put
    //.acquire_$error exec /newerror //true put
    //.acquire_$error exec exch /errorname exch put
    //.acquire_$error exec exch /command exch put          % <-- /command
    //.acquire_$error exec /errorinfo known not
        { //.acquire_$error exec /errorinfo //null put } if
    //.acquire_$error exec /recordstacks get
    //.acquire_$error exec /errorname get /VMerror ne and
        { %% Attempt to store the stack contents atomically.
          count array astore dup //.acquire_$error exec /ostack 4 -1 roll
          countexecstack array execstack
            dup length 2 sub 0 exch getinterval     %% strip handler frames
          //.acquire_$error exec /estack 3 -1 roll
          countdictstack array dictstack //.acquire_$error exec /dstack 3 -1 roll
          put put put aload pop
        } { ... clear stacks ... } ifelse
    ...
    stop
} .internalbind def
```

Key shapes:

- `$error /command` is the failing object — `IREF` at fault
  time. This is what `==` shows in error messages.
- `$error /errorname` is the symbolic error.
- `$error /ostack`, `$error /estack`, `$error /dstack` are
  snapshots of all three stacks captured atomically. They are
  arrays of refs.
- The `execstack` snapshot strips the top two frames (the
  handler's own ostack/estack manipulation) via
  `getinterval` so it shows the call site as the top, not the
  handler.

`signalerror` (`gs_init.ps:893`) is a one-liner that looks up
the named handler in `errordict` and runs it.

### 8.4 What's actually in `/estack` after an error

Because gs never pushed individual operator refs, the e-stack
snapshot contains:

- The procedure frames the dispatcher had stored via
  `store_state` on entry to inner procs.
- Continuation operators sitting between iterations
  (`for_pos_int_continue`, `repeat_continue`,
  `loop_continue`, `oparray_pop`, etc.).
- `t_null` marks set by `push_mark_estack(es_for, ...)` etc.
- The failing operator object is in `$error /command`, not
  on the e-stack.

This is what SLI-level traceback printing shows. `pstack`,
`=stack`, the default `handleerror` printer (`gs_init.ps:1250-
1253`) — all walk this snapshot.

---

## 9. How the topology compares to sli3

For reference when planning sli3 changes:

| Aspect | gs | sli3 today |
|---|---|---|
| Interpreter shape | One `interp()` function; one switch on `r_type_xe`; main loop walks body inline | `execute_dispatch_` with one switch on `tag()`; "I'm walking a body" is a *case* in the switch (`iiteratetype` etc.); each token re-enters the outer switch |
| Per-token outer-loop cost | `next()` macro → `goto top` (1-2 insns, register state) | `goto start_repeat` then iter-case header re-loads proc/pos from e-stack picks |
| Hot-op bodies | Inlined as switch arms (`tx_op_*`) | `SLIFunction::execute` virtual call |
| Operator self-pop | None — dispatcher tracks current op via register `IREF` | Each `SLIFunction::execute` ends with `i->EStack().pop()` (~316 sites) |
| Error attribution | `IREF` captured at fault; pushed to operand stack as `<command>` | E-stack top is the failing op Token; `raiseerror` reads its name |
| Name lookup cache | `name->pvalue` pointer (1 indirection, common case) | Dictstack lookup via `lookup()` (more indirection) |
| Procedure body storage | `t_array` (full refs) or `t_mixedarray`/`t_shortarray` (2-byte packed refs) | `TokenArray` (16-byte Tokens only) |
| Loop entry | C operator pushes mark+state+continuation op onto e-stack, returns `o_push_estack` | C operator pushes [mark, count, proc, pos, repeattype] onto e-stack, returns; dispatcher's iter case runs the body |
| Loop continuation | Continuation op (`repeat_continue` etc.) executes between iters; pushes another proc copy | Iter-case state machine handles position + counter inline |

---

## 10. File pointers

For quick navigation when revisiting:

| What | Path |
|---|---|
| `ref` struct | `psi/iref.h:412-457` |
| Type IDs | `psi/iref.h:124-208` |
| Attribute bits | `psi/iref.h:309-365` |
| Packed-ref encoding | `psi/ipacked.h:24-125` |
| Stack data struct | `psi/isdata.h:67-87` |
| Stack block | `psi/istack.h:51-55` |
| Operand-stack macros | `psi/ostack.h:25-71` |
| Name struct + pvalue cache | `psi/inamedef.h:54-68` |
| Top of `interp()` | `psi/interp.c:947-1100` |
| `top:` switch | `psi/interp.c:1100-1525` |
| Hot operators inlined | `psi/interp.c:1213-1317` |
| Generic operator dispatch | `psi/interp.c:1362-1420` |
| Name dispatch | `psi/interp.c:1421-1522` |
| Body-walk macros (`next`, ...) | `psi/interp.c:1066-1090` |
| Procedure entry (`prst:` / `pr:`) | `psi/interp.c:1339-1361` |
| Tail-recursion (`bot:`/`out:`/`up:`) | `psi/interp.c:1853-1882` |
| Error capture (`rwe:` / `rwei:`) | `psi/interp.c:1925-1965` |
| Top-level error loop | `psi/interp.c:464-762` |
| `zrepeat` + `repeat_continue` | `psi/zcontrol.c:464-498` |
| `zfor` + `for_*_continue` | `psi/zcontrol.c:288-405` |
| `zloop` + `loop_continue` | `psi/zcontrol.c:501-527` |
| `zexit` (e-stack walker) | `psi/zcontrol.c:531-557` |
| `zbind` | `psi/zmisc.c:46-166` |
| `.errorhandler` (PostScript) | `Resource/Init/gs_init.ps:1158-1201` |
| `signalerror` (PostScript) | `Resource/Init/gs_init.ps:893-894` |
| Default `handleerror` printer | `Resource/Init/gs_init.ps:1207+` |

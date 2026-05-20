// Axis II: inline body helpers for hot operators.
//
// Single source of truth: the operator class's execute() method
// delegates here, and the dispatcher's hot-op switch arms call
// the same helpers. Keep these bodies tiny -- they're inlined
// into the dispatcher's body-walk loop, where register pressure
// is high and any non-trivial work spills the operand-stack
// pointer.
//
// Each helper has the same contract as the SLIFunction's
// execute(): pre: ostack precondition met; post: result on
// ostack; on type/range error: i->raiseerror(...) and return.
// Helpers don't pop their own e-stack slot (the dispatcher
// pre-pops under the post-Axis-I ABI). Control-flow ops like
// `if` and `def` still push/consume e-stack frames as part of
// their semantics -- see hot_op_if.

#ifndef SLI_OP_BODIES_H
#define SLI_OP_BODIES_H

#include "sli_access_check.h"
#include "sli_interpreter.h"
#include "sli_tokenstack.h"

namespace sli3
{

// Resolve an SLI container index against `size`, supporting
// Python-style negative indexing (-1 == last element). Returns the
// resolved non-negative index in [0, size), or -1 if the original
// index is out of range. Caller raises RangeCheck on -1.
static inline long resolve_index(long idx, size_t size)
{
    if (idx < 0)
        idx += static_cast<long>(size);
    if (idx < 0 || static_cast<size_t>(idx) >= size)
        return -1;
    return idx;
}

// pop -- discard ostack top.
static inline void hot_op_pop(SLIInterpreter* i)
{
    i->require_stack_load(1);
    i->pop();
}

// dup -- push a copy of ostack top.
static inline void hot_op_dup(SLIInterpreter* i)
{
    i->require_stack_load(1);
    i->OStack().index(0);
}

// exch -- swap top two ostack slots.
static inline void hot_op_exch(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->OStack().swap();
}

// add_ii -- integer add with overflow check.
// pick(1) <- pick(1) + pick(0); pop.
static inline void hot_op_add_ii(SLIInterpreter* i)
{
    i->require_stack_load(2);
    long out;
    if (__builtin_add_overflow(i->pick(1).data_.long_val,
                                i->pick(0).data_.long_val,
                                &out))
    {
        i->raiseerror(i->RangeCheckError);
        return;
    }
    i->pick(1).data_.long_val = out;
    i->pop();
}

// add -- poly-arithmetic dispatcher (int+int, dd, di, id).
// Same body as AddFunction::execute; the standalone method
// delegates here.
static inline void hot_op_add(SLIInterpreter* i)
{
    i->require_stack_load(2);
    Token& a = i->pick(1);
    Token& b = i->pick(0);
    unsigned ta = a.tag();
    unsigned tb = b.tag();
    if (ta == sli3::integertype && tb == sli3::integertype) {
        long out;
        if (__builtin_add_overflow(a.data_.long_val, b.data_.long_val, &out)) {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        a.data_.long_val = out;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::doubletype) {
        a.data_.double_val += b.data_.double_val;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::integertype) {
        a.data_.double_val += static_cast<double>(b.data_.long_val);
        i->pop();
        return;
    }
    if (ta == sli3::integertype && tb == sli3::doubletype) {
        a.data_.double_val =
            static_cast<double>(a.data_.long_val) + b.data_.double_val;
        a.type_ = b.type_;
        i->pop();
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// sub -- poly-arithmetic dispatcher, same shape as hot_op_add.
static inline void hot_op_sub(SLIInterpreter* i)
{
    i->require_stack_load(2);
    Token& a = i->pick(1);
    Token& b = i->pick(0);
    unsigned ta = a.tag();
    unsigned tb = b.tag();
    if (ta == sli3::integertype && tb == sli3::integertype) {
        long out;
        if (__builtin_sub_overflow(a.data_.long_val, b.data_.long_val, &out)) {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        a.data_.long_val = out;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::doubletype) {
        a.data_.double_val -= b.data_.double_val;
        i->pop();
        return;
    }
    if (ta == sli3::doubletype && tb == sli3::integertype) {
        a.data_.double_val -= static_cast<double>(b.data_.long_val);
        i->pop();
        return;
    }
    if (ta == sli3::integertype && tb == sli3::doubletype) {
        a.data_.double_val =
            static_cast<double>(a.data_.long_val) - b.data_.double_val;
        a.type_ = b.type_;
        i->pop();
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// if -- bool proc -> push proc on e-stack if bool is true.
// Under the new ABI, the dispatcher pre-popped /if's own slot
// (in the main case) or never pushed it (in the body-walk
// fast path). The body just consumes the two ostack args.
// Swap-out (cf. IfelseFunction): when taking the branch, move
// the proc Token to the estack via swap to skip one
// ArrayType::add_reference / remove_reference pair.
static inline void hot_op_if(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->require_stack_type(1, sli3::booltype);
    if (i->pick(1).data_.bool_val) {
        i->EStack().push(Token{});
        i->EStack().top().swap(i->top());
    }
    i->pop(2);
}

// def -- /lit obj -> store under name in current dict.
static inline void hot_op_def(SLIInterpreter* i)
{
    i->require_stack_load(2);
    i->require_stack_type(1, sli3::literaltype);
    // def mutates the current top-of-dictstack dict. Refuse if it
    // has been narrowed to readonly (e.g. systemdict after bootstrap).
    Dictionary* top = i->DStack().top();
    if (top && !require_writable(top, i)) return;
    i->def(i->pick(1).data_.name_val, i->top());
    i->pop(2);
}

// ---------------------------------------------------------------------
// Axis II step 3: comparison / lookup hot ops.
// ---------------------------------------------------------------------

// Common body for gt / lt / geq / leq. Handles 4 numeric arms +
// string-prefix arm. Writes bool result to slot 1, pops slot 0.
template <typename CmpNum, typename CmpStr>
static inline void hot_op_compare(SLIInterpreter* i, CmpNum cn, CmpStr cs)
{
    i->require_stack_load(2);
    Token& a = i->pick(1);
    Token& b = i->pick(0);
    unsigned ta = a.tag();
    unsigned tb = b.tag();
    SLIType *bool_tid = i->get_type(sli3::booltype);

    bool result;
    if (ta == sli3::integertype && tb == sli3::integertype)
        result = cn(a.data_.long_val, b.data_.long_val);
    else if (ta == sli3::doubletype && tb == sli3::doubletype)
        result = cn(a.data_.double_val, b.data_.double_val);
    else if (ta == sli3::doubletype && tb == sli3::integertype)
        result = cn(a.data_.double_val, static_cast<double>(b.data_.long_val));
    else if (ta == sli3::integertype && tb == sli3::doubletype)
        result = cn(static_cast<double>(a.data_.long_val), b.data_.double_val);
    else if (ta == sli3::stringtype && tb == sli3::stringtype)
    {
        if (a.data_.string_val == nullptr || b.data_.string_val == nullptr)
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        result = cs(a.data_.string_val->str(), b.data_.string_val->str());
        // String comparison: slot 1 holds a refcounted string Token
        // whose type slot is about to be overwritten as booltype.
        // clear() releases the refcount before that overwrite;
        // without it the string leaks until process exit.
        a.clear();
    }
    else
    {
        // Unsupported operand combination. The dispatcher's hot arm
        // calls this without trie filtering, so we must reject here.
        i->raiseerror(i->ArgumentTypeError);
        return;
    }
    a.type_ = bool_tid;
    a.data_.bool_val = result;
    i->pop();
}

static inline void hot_op_gt(SLIInterpreter* i)
{
    hot_op_compare(i,
        [](auto x, auto y) { return x > y; },
        [](auto const& x, auto const& y) { return x > y; });
}

static inline void hot_op_lt(SLIInterpreter* i)
{
    hot_op_compare(i,
        [](auto x, auto y) { return x < y; },
        [](auto const& x, auto const& y) { return x < y; });
}

static inline void hot_op_geq(SLIInterpreter* i)
{
    hot_op_compare(i,
        [](auto x, auto y) { return x >= y; },
        [](auto const& x, auto const& y) { return x >= y; });
}

static inline void hot_op_leq(SLIInterpreter* i)
{
    hot_op_compare(i,
        [](auto x, auto y) { return x <= y; },
        [](auto const& x, auto const& y) { return x <= y; });
}

// PostScript spec: `eq` / `ne` on a composite operand requires read
// access. Scalars (integer, double, bool, name, …) are unaffected.
// Strings are the only composite whose equal_tokens reads contents
// in sli3 — array/dict eq compares pointer identity, so reading the
// underlying payload isn't required. Gate the string arm here so the
// composite-typed `compare()` virtual stays oblivious to access.
static inline bool eq_check_string_access(SLIInterpreter* i)
{
    Token const& a = i->pick(1);
    Token const& b = i->pick(0);
    if (a.tag() == sli3::stringtype
        && a.data_.string_val != nullptr
        && !require_readable(a.data_.string_val, i))
        return false;
    if (b.tag() == sli3::stringtype
        && b.data_.string_val != nullptr
        && !require_readable(b.data_.string_val, i))
        return false;
    return true;
}

// eq -- any/any. Dispatches via Token::operator== (which is a
// virtual call on the type's equal_tokens). Slot-1 must clear()
// before being overwritten as a bool — otherwise refcounted
// payloads (string/array/dict) leak: the future destructor sees
// type_=booltype and skips the refcount decrement.
static inline void hot_op_eq(SLIInterpreter* i)
{
    i->require_stack_load(2);
    if (!eq_check_string_access(i)) return;
    SLIType *bool_tid = i->get_type(sli3::booltype);
    bool result = i->pick(1) == i->pick(0);
    i->pop();
    i->top().clear();
    i->top().type_ = bool_tid;
    i->top().data_.bool_val = result;
}

// neq -- inverse of eq.
static inline void hot_op_neq(SLIInterpreter* i)
{
    i->require_stack_load(2);
    if (!eq_check_string_access(i)) return;
    SLIType *bool_tid = i->get_type(sli3::booltype);
    bool result = not (i->pick(1) == i->pick(0));
    i->pop();
    i->top().clear();
    i->top().type_ = bool_tid;
    i->top().data_.bool_val = result;
}

// get -- container key -> elem. Multi-arm dispatch. Mirrors
// GetFunction::execute (sli_container_ops.cpp). Inlined here so
// the dispatcher's hot-op switch can fold the per-tag branches
// into the body-walk loop.
static inline void hot_op_get(SLIInterpreter* i)
{
    i->require_stack_load(2);
    unsigned coll = i->pick(1).tag();
    unsigned key  = i->pick(0).tag();

    // Array-like (array / procedure / litprocedure) indexed by int.
    if ((coll == sli3::arraytype
         || coll == sli3::proceduretype
         || coll == sli3::litproceduretype)
        && key == sli3::integertype)
    {
        TokenArray* arr = i->pick(1).data_.array_val;
        if (!require_readable(arr, i)) return;
        long idx = resolve_index(i->top().data_.long_val, arr->size());
        if (idx < 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        Token elem = arr->get(idx);
        i->pop(2);
        i->push(elem);
        return;
    }
    if (coll == sli3::arraytype && key == sli3::arraytype)
    {
        // array of indices -> array of elements.
        TokenArray* arr  = i->pick(1).data_.array_val;
        TokenArray* idxs = i->top().data_.array_val;
        if (!require_readable(arr, i)) return;
        if (!require_readable(idxs, i)) return;
        TokenArray* out  = new TokenArray();
        out->reserve(idxs->size());
        for (Token const* t = idxs->begin(); t != idxs->end(); ++t)
        {
            if (!t->is_of_type(sli3::integertype))
            {
                delete out;
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
            long k = resolve_index(t->data_.long_val, arr->size());
            if (k < 0)
            {
                delete out;
                i->raiseerror(i->RangeCheckError);
                return;
            }
            out->push_back(arr->get(k));
        }
        i->pop(2);
        i->push(i->new_token<sli3::arraytype>(out));
        return;
    }
    if (coll == sli3::stringtype && key == sli3::integertype)
    {
        SLIString* sv = i->pick(1).data_.string_val;
        if (!require_readable(sv, i)) return;
        std::string& s = sv->str();
        long idx = resolve_index(i->top().data_.long_val, s.size());
        if (idx < 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        long c = static_cast<unsigned char>(s[idx]);
        i->pop(2);
        i->push(c);
        return;
    }
    if (coll == sli3::dictionarytype && key == sli3::literaltype)
    {
        Dictionary* d = i->pick(1).data_.dict_val;
        if (!require_readable(d, i)) return;
        Name n(i->top().data_.name_val);
        if (!d->known(n))
        {
            i->raiseerror(i->UndefinedNameError);
            return;
        }
        Token elem = d->lookup(n);
        i->pop(2);
        i->push(elem);
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// put -- container key value -> [container]. Multi-arm dispatch.
// Mirrors PutFunction::execute (sli_container_ops.cpp). NEST 2.x
// convention for array/proc/string variants: consumes (idx, value)
// and leaves container on top. Dict variant consumes all three.
static inline void hot_op_put(SLIInterpreter* i)
{
    i->require_stack_load(3);
    unsigned coll = i->pick(2).tag();
    unsigned idx  = i->pick(1).tag();

    if ((coll == sli3::arraytype
         || coll == sli3::proceduretype
         || coll == sli3::litproceduretype)
        && idx == sli3::integertype)
    {
        // /put assigns into an existing slot; PostScript and NEST 2.x
        // preserve the mutation through every holder of the array. No
        // clone-on-write here — that's what /append uses to dodge the
        // parser-shared-literal trample.
        TokenArray* arr = i->pick(2).data_.array_val;
        if (!require_writable(arr, i)) return;
        long k = resolve_index(i->pick(1).data_.long_val, arr->size());
        if (k < 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        (*arr)[static_cast<size_t>(k)] = i->top();
        i->pop(2);
        return;
    }
    if (coll == sli3::arraytype && idx == sli3::arraytype)
    {
        // Path-into-nested-arrays: `arr [i0 i1 ...] val put` →
        // `arr[i0][i1]... = val`. Mirrors PutArrayArrayTokenFunction.
        // Path-put does NOT clone-on-write down the descent chain: a
        // shared intermediate array would leak the leaf mutation to
        // other holders. The single-level array put above privatises
        // its slot; nested-path COW would require cloning every level.
        // Not in scope for typical SLI usage (paths land on dict-side
        // structures); revisit if a regression surfaces.
        TokenArray* src  = i->pick(2).data_.array_val;
        TokenArray* path = i->pick(1).data_.array_val;
        if (!require_readable(path, i)) return;
        if (path->empty())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* cur = src;
        for (Token const* it = path->begin(); it != path->end(); ++it)
        {
            if (!it->is_of_type(sli3::integertype))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
            // Reading cur[k] for descent (and for the leaf write
            // target check below) is a read of cur — gate it.
            // Intermediate executeonly/noaccess arrays must reject
            // descent before exposing element contents.
            if (!require_readable(cur, i)) return;
            long k = resolve_index(it->data_.long_val, cur->size());
            if (k < 0)
            {
                i->raiseerror(i->RangeCheckError);
                return;
            }
            if (it + 1 == path->end())
            {
                // Only the leaf is mutated — check writability there.
                if (!require_writable(cur, i)) return;
                (*cur)[static_cast<size_t>(k)] = i->top();
            }
            else
            {
                Token& next = (*cur)[static_cast<size_t>(k)];
                if (!next.is_of_type(sli3::arraytype))
                {
                    i->raiseerror(i->ArgumentTypeError);
                    return;
                }
                cur = next.data_.array_val;
            }
        }
        i->pop(2);
        return;
    }
    if (coll == sli3::stringtype && idx == sli3::integertype)
    {
        SLIString* sv = i->pick(2).data_.string_val;
        if (!require_writable(sv, i)) return;
        std::string& s = sv->str();
        long k = resolve_index(i->pick(1).data_.long_val, s.size());
        long c = i->top().data_.long_val;
        if (k < 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        s[static_cast<size_t>(k)] = static_cast<char>(c);
        i->pop(2);
        return;
    }
    if (coll == sli3::dictionarytype && idx == sli3::literaltype)
    {
        Dictionary* d = i->pick(2).data_.dict_val;
        if (!require_writable(d, i)) return;
        Name n(i->pick(1).data_.name_val);
        // Insert into the dict directly. If this dict is currently on
        // the dictstack the lookup cache may hold a stale Token* for
        // the same name resolving to a SHADOWED binding in a lower
        // dict (or a nullptr from a prior miss). The DictionaryStack's
        // def() path does this invalidation; hot_op_put inlines the
        // insert, so we replicate the invalidation here.
        if (d->is_on_dictstack())
        {
            i->DStack().clear_token_from_cache(n);
            // basecache_ is only populated for the base dict; the
            // helper is a no-op when n isn't cached there, so it's
            // safe to call unconditionally.
            i->DStack().clear_token_from_basecache(n);
        }
        d->insert(n, i->top());
        i->pop(3);
        return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

}  // namespace sli3

#endif  // SLI_OP_BODIES_H

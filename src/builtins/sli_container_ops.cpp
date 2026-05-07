// Type-specific container operators consumed by sli-init.sli /
// typeinit.sli when building the trie machinery. Names match the
// NEST 2.x convention `<op>_<type>` so the vendored .sli files load
// unchanged.
//
// Operators provided here:
//   length_p / length_lp / length_a / length_s / length_d
//   get_a / get_a_a / get_p / get_lp / get_s / get_d / get_d_a
//   put_a / put_p / put_lp / put_s / put_d / put_a_a_t

#include "sli_array.h"
#include "sli_container_ops.h"
#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include "sli_function.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

namespace sli3
{
namespace
{

//------------------------------------------------------------------------
// length_*
//------------------------------------------------------------------------

template <unsigned int TID>
class LengthArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, TID);
        long n = static_cast<long>(i->top().data_.array_val->size());
        i->pop();
        i->push(n);
        i->EStack().pop();
    }
};

class LengthStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        long n = static_cast<long>(i->top().data_.string_val->size());
        i->pop();
        i->push(n);
        i->EStack().pop();
    }
};

class LengthDictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        long n = static_cast<long>(i->top().data_.dict_val->size());
        i->pop();
        i->push(n);
        i->EStack().pop();
    }
};

//------------------------------------------------------------------------
// get_a / get_p / get_lp — array-backed types: container int -> elem
//------------------------------------------------------------------------

template <unsigned int TID>
class GetArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, TID);
        i->require_stack_type(0, sli3::integertype);
        TokenArray* arr = i->pick(1).data_.array_val;
        long idx = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= arr->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        Token elem = arr->get(idx);
        i->pop(2);
        i->push(elem);
        i->EStack().pop();
    }
};

// get_a_a — array of indices into an array, returns array of elems.
class GetArrayArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::arraytype);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* arr = i->pick(1).data_.array_val;
        TokenArray* idxs = i->top().data_.array_val;
        TokenArray* out = new TokenArray();
        out->reserve(idxs->size());
        for (Token const* t = idxs->begin(); t != idxs->end(); ++t)
        {
            if (!t->is_of_type(sli3::integertype))
            {
                delete out;
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
            long k = t->data_.long_val;
            if (k < 0 || static_cast<size_t>(k) >= arr->size())
            {
                delete out;
                i->raiseerror(i->RangeCheckError);
                return;
            }
            out->push_back(arr->get(k));
        }
        i->pop(2);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// get_s — string int -> int (the character code).
class GetStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::integertype);
        std::string& s = i->pick(1).data_.string_val->str();
        long idx = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= s.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        long c = static_cast<unsigned char>(s[idx]);
        i->pop(2);
        i->push(c);
        i->EStack().pop();
    }
};

// get_d — dict literal -> elem
class GetDictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::dictionarytype);
        i->require_stack_type(0, sli3::literaltype);
        Dictionary* d = i->pick(1).data_.dict_val;
        Name n(i->top().data_.name_val);
        if (!d->known(n))
        {
            i->raiseerror(i->UndefinedNameError);
            return;
        }
        Token elem = d->lookup(n);
        i->pop(2);
        i->push(elem);
        i->EStack().pop();
    }
};

//------------------------------------------------------------------------
// put_*
//------------------------------------------------------------------------

template <unsigned int TID>
class PutArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        // NEST 2.x convention: `array index value put_a -> array`.
        // Consumes only the (idx, value) pair and leaves the array on
        // top, so `bind`'s `2 copy ... put` pattern preserves the
        // for-loop's [proc, i] state across iterations.
        i->require_stack_load(3);
        i->require_stack_type(2, TID);
        i->require_stack_type(1, sli3::integertype);
        TokenArray* arr = i->pick(2).data_.array_val;
        long idx = i->pick(1).data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= arr->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        (*arr)[static_cast<size_t>(idx)] = i->top();
        i->pop(2);
        i->EStack().pop();
    }
};

class PutStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        std::string& s = i->pick(2).data_.string_val->str();
        long idx = i->pick(1).data_.long_val;
        long c = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= s.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        s[static_cast<size_t>(idx)] = static_cast<char>(c);
        i->pop(2);
        i->EStack().pop();
    }
};

class PutDictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::dictionarytype);
        i->require_stack_type(1, sli3::literaltype);
        Dictionary* d = i->pick(2).data_.dict_val;
        Name n(i->pick(1).data_.name_val);
        d->insert(n, i->top());
        i->pop(3);  // dict / key / value all consumed
        i->EStack().pop();
    }
};

// put_a_a_t — nested-array put: walks an index path into a nested array
// and replaces the leaf. Mirrors the legacy NEST behaviour.
class PutArrayArrayTokenFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::arraytype);
        i->require_stack_type(1, sli3::arraytype);
        TokenArray* src = i->pick(2).data_.array_val;
        TokenArray* path = i->pick(1).data_.array_val;
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
            long k = it->data_.long_val;
            if (k < 0 || static_cast<size_t>(k) >= cur->size())
            {
                i->raiseerror(i->RangeCheckError);
                return;
            }
            if (it + 1 == path->end())
            {
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
        i->pop(2);  // drop path + value, keep array
        i->EStack().pop();
    }
};

LengthArrayLikeFunction<sli3::proceduretype>          length_p_fn;
LengthArrayLikeFunction<sli3::litproceduretype>       length_lp_fn;
LengthArrayLikeFunction<sli3::arraytype>              length_a_fn;
LengthStringFunction                                  length_s_fn;
LengthDictFunction                                    length_d_fn;

GetArrayLikeFunction<sli3::arraytype>                 get_a_fn;
GetArrayLikeFunction<sli3::proceduretype>             get_p_fn;
GetArrayLikeFunction<sli3::litproceduretype>          get_lp_fn;
GetArrayArrayFunction                                 get_a_a_fn;
GetStringFunction                                     get_s_fn;
GetDictFunction                                       get_d_fn;

PutArrayLikeFunction<sli3::arraytype>                 put_a_fn;
PutArrayLikeFunction<sli3::proceduretype>             put_p_fn;
PutArrayLikeFunction<sli3::litproceduretype>          put_lp_fn;
PutStringFunction                                     put_s_fn;
PutDictFunction                                       put_d_fn;
PutArrayArrayTokenFunction                            put_a_a_t_fn;

//------------------------------------------------------------------------
// String ops
//------------------------------------------------------------------------

// `s1 s2 join_s -> s` — concatenation. Result on top, both inputs gone.
class JoinStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& a = i->pick(1).data_.string_val->str();
        std::string const& b = i->top().data_.string_val->str();
        std::string out;
        out.reserve(a.size() + b.size());
        out.append(a).append(b);
        i->pop(2);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `haystack needle search_s -> post needle pre true | haystack false`
// Decomposes haystack around the first occurrence of needle. Mirrors
// PostScript `search`. The "post" is what's left of the haystack after
// the match; ordering matches NEST 2.x.
class SearchStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::stringtype);
        std::string haystack = i->pick(1).data_.string_val->str();  // copy
        std::string needle   = i->top().data_.string_val->str();    // copy
        i->pop(2);
        auto pos = haystack.find(needle);
        if (pos == std::string::npos)
        {
            i->push(i->new_token<sli3::stringtype, std::string>(std::move(haystack)));
            i->push<bool>(false);
        }
        else
        {
            std::string pre  = haystack.substr(0, pos);
            std::string post = haystack.substr(pos + needle.size());
            i->push(i->new_token<sli3::stringtype, std::string>(std::move(post)));
            i->push(i->new_token<sli3::stringtype, std::string>(std::move(needle)));
            i->push(i->new_token<sli3::stringtype, std::string>(std::move(pre)));
            i->push<bool>(true);
        }
        i->EStack().pop();
    }
};

// `int cvi_s -> string` and `double cvd_s -> string`
class CviStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::integertype);
        long v = i->top().data_.long_val;
        i->pop();
        i->push(i->new_token<sli3::stringtype, std::string>(std::to_string(v)));
        i->EStack().pop();
    }
};

class CvdStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::doubletype);
        double v = i->top().data_.double_val;
        i->pop();
        i->push(i->new_token<sli3::stringtype, std::string>(std::to_string(v)));
        i->EStack().pop();
    }
};

// `arr index count getinterval_a -> subarr`
// Returns a fresh arraytype with elements [index, index+count). Validation
// matches NEST 2.20.2 sli/slidata.cc (Getinterval_aFunction):
//   count < 0           -> PositiveIntegerExpectedError
//   index < 0           -> RangeCheckError
//   index >= size       -> RangeCheckError  (so empty source always fails)
//   index + count > size -> RangeCheckError
class GetintervalArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::arraytype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        long count = i->top().data_.long_val;
        long idx   = i->pick(1).data_.long_val;
        TokenArray* arr = i->pick(2).data_.array_val;
        if (count < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        if (idx < 0
            || static_cast<size_t>(idx) >= arr->size()
            || static_cast<size_t>(idx) + static_cast<size_t>(count) > arr->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* out = new TokenArray();
        out->reserve(static_cast<size_t>(count));
        for (long k = 0; k < count; ++k)
            out->push_back(arr->get(static_cast<size_t>(idx + k)));
        i->pop(3);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// `str index count getinterval_s -> substr`. Same validation as the array
// variant; result is a fresh stringtype.
class GetintervalStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        long count = i->top().data_.long_val;
        long idx   = i->pick(1).data_.long_val;
        std::string const& s = i->pick(2).data_.string_val->str();
        if (count < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        if (idx < 0
            || static_cast<size_t>(idx) >= s.size()
            || static_cast<size_t>(idx) + static_cast<size_t>(count) > s.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        std::string out = s.substr(static_cast<size_t>(idx),
                                   static_cast<size_t>(count));
        i->pop(3);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `a1 a2 join_a -> a1++a2` — concatenate two arrays. Result is a fresh
// arraytype. NEST 2.20.2 sli/slidata.cc mutates a1 in place via
// append_move; we copy to keep refcount aliasing simple.
class JoinArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::arraytype);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray const* a = i->pick(1).data_.array_val;
        TokenArray const* b = i->top().data_.array_val;
        TokenArray* out = new TokenArray();
        out->reserve(a->size() + b->size());
        for (size_t k = 0; k < a->size(); ++k) out->push_back(a->get(k));
        for (size_t k = 0; k < b->size(); ++k) out->push_back(b->get(k));
        i->pop(2);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// `a1 idx a2 insert_a -> a1'` — insert all elements of a2 into a1
// starting at position idx; existing elements at idx and beyond shift
// right. Result is a fresh arraytype. NEST 2.20.2 sli/slidata.cc uses
// a1->insert_move(idx, a2); we copy.
//
// Validation per NEST: idx >= 0 && idx < a1.size() (strict — idx==size
// rejected; empty source always rejected). Out-of-range raises
// RangeCheckError.
class InsertArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::arraytype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray const* a1 = i->pick(2).data_.array_val;
        long idx = i->pick(1).data_.long_val;
        TokenArray const* a2 = i->top().data_.array_val;
        if (idx < 0 || static_cast<size_t>(idx) >= a1->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* out = new TokenArray();
        out->reserve(a1->size() + a2->size());
        size_t pos = static_cast<size_t>(idx);
        for (size_t k = 0; k < pos; ++k)         out->push_back(a1->get(k));
        for (size_t k = 0; k < a2->size(); ++k)  out->push_back(a2->get(k));
        for (size_t k = pos; k < a1->size(); ++k) out->push_back(a1->get(k));
        i->pop(3);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// `s1 idx s2 insert_s -> s1'` — same shape as insert_a, stringtype payload.
class InsertStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& s1 = i->pick(2).data_.string_val->str();
        long idx = i->pick(1).data_.long_val;
        std::string const& s2 = i->top().data_.string_val->str();
        if (idx < 0 || static_cast<size_t>(idx) >= s1.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        std::string out;
        out.reserve(s1.size() + s2.size());
        size_t pos = static_cast<size_t>(idx);
        out.append(s1, 0, pos);
        out.append(s2);
        out.append(s1, pos, std::string::npos);
        i->pop(3);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `a1 idx n a2 replace_a -> a1'` — replace n elements of a1 starting
// at idx with the contents of a2. Result is a fresh arraytype. NEST
// 2.20.2 sli/slidata.cc uses a1->replace_move(idx, n, a2).
//
// Validation per NEST:
//   idx < 0 || idx >= a1.size() -> RangeCheckError (idx==size rejected)
//   n   < 0                     -> PositiveIntegerExpectedError
// idx + n > size silently clamps (matches std::string::replace and
// the underlying TokenArrayObj behaviour).
class ReplaceArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        i->require_stack_type(3, sli3::arraytype);
        i->require_stack_type(2, sli3::integertype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray const* a1 = i->pick(3).data_.array_val;
        long idx = i->pick(2).data_.long_val;
        long n   = i->pick(1).data_.long_val;
        TokenArray const* a2 = i->top().data_.array_val;
        if (idx < 0 || static_cast<size_t>(idx) >= a1->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        size_t pos = static_cast<size_t>(idx);
        size_t cnt = static_cast<size_t>(n);
        if (pos + cnt > a1->size()) cnt = a1->size() - pos;
        TokenArray* out = new TokenArray();
        out->reserve(a1->size() - cnt + a2->size());
        for (size_t k = 0; k < pos; ++k)               out->push_back(a1->get(k));
        for (size_t k = 0; k < a2->size(); ++k)        out->push_back(a2->get(k));
        for (size_t k = pos + cnt; k < a1->size(); ++k) out->push_back(a1->get(k));
        i->pop(4);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// `s1 idx n s2 replace_s -> s1'` — same shape as replace_a, stringtype.
class ReplaceStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(4);
        i->require_stack_type(3, sli3::stringtype);
        i->require_stack_type(2, sli3::integertype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& s1 = i->pick(3).data_.string_val->str();
        long idx = i->pick(2).data_.long_val;
        long n   = i->pick(1).data_.long_val;
        std::string const& s2 = i->top().data_.string_val->str();
        if (idx < 0 || static_cast<size_t>(idx) >= s1.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        // std::string::replace already clamps n to remaining length.
        std::string out = s1;
        out.replace(static_cast<size_t>(idx), static_cast<size_t>(n), s2);
        i->pop(4);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `c1 idx n erase_<a|p> -> c1'` — remove n elements of c1 starting
// at idx. Result is a fresh container of the same SLI type. NEST
// 2.20.2 sli/slidata.cc uses c1->erase(idx, n) on the underlying
// TokenArray (same impl shared by erase_a and erase_p; differs only
// in typeid).
//
// Validation per NEST:
//   idx < 0 || idx >= c1.size() -> RangeCheckError
//   n   < 0                     -> PositiveIntegerExpectedError
// idx+n > size silently clamps to remaining length.
class EraseArrayLikeFunction : public SLIFunction
{
    sli3::sli_typeid tid_;
public:
    explicit EraseArrayLikeFunction(sli3::sli_typeid t) : tid_(t) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, tid_);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        TokenArray const* a1 = i->pick(2).data_.array_val;
        long idx = i->pick(1).data_.long_val;
        long n   = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= a1->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        size_t pos = static_cast<size_t>(idx);
        size_t cnt = static_cast<size_t>(n);
        if (pos + cnt > a1->size()) cnt = a1->size() - pos;
        TokenArray* out = new TokenArray();
        out->reserve(a1->size() - cnt);
        for (size_t k = 0; k < pos; ++k)               out->push_back(a1->get(k));
        for (size_t k = pos + cnt; k < a1->size(); ++k) out->push_back(a1->get(k));
        i->pop(3);
        // Build the result Token directly (the new_token specialization
        // is keyed by static typeid, but we need the runtime tid_).
        Token t(i->get_type(tid_));
        t.data_.array_val = out;
        i->push(t);
        i->EStack().pop();
    }
};

// `s1 idx n erase_s -> s1'` — same shape, stringtype payload.
class EraseStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        std::string const& s1 = i->pick(2).data_.string_val->str();
        long idx = i->pick(1).data_.long_val;
        long n   = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= s1.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        std::string out = s1;
        out.erase(static_cast<size_t>(idx), static_cast<size_t>(n));  // clamps
        i->pop(3);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `arr idx tok insertelement_a -> arr'` — insert a single token at
// position idx of arr. NEST 2.20.2 sli/slidata.cc Insert_Element_a uses
// a1->insert_move(idx, top()). idx is strict in [0, size).
class InsertElementArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::arraytype);
        i->require_stack_type(1, sli3::integertype);
        TokenArray const* a1 = i->pick(2).data_.array_val;
        long idx = i->pick(1).data_.long_val;
        Token const& elem = i->top();
        if (idx < 0 || static_cast<size_t>(idx) >= a1->size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* out = new TokenArray();
        out->reserve(a1->size() + 1);
        size_t pos = static_cast<size_t>(idx);
        for (size_t k = 0; k < pos; ++k)         out->push_back(a1->get(k));
        out->push_back(elem);
        for (size_t k = pos; k < a1->size(); ++k) out->push_back(a1->get(k));
        i->pop(3);
        i->push(i->new_token<sli3::arraytype>(out));
        i->EStack().pop();
    }
};

// `str idx ch insertelement_s -> str'` — insert one character (int
// cast to char) at position idx. Validation as for the array form.
class InsertElementStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::stringtype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        std::string const& s1 = i->pick(2).data_.string_val->str();
        long idx = i->pick(1).data_.long_val;
        long ch  = i->top().data_.long_val;
        if (idx < 0 || static_cast<size_t>(idx) >= s1.size())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        std::string out = s1;
        out.insert(static_cast<size_t>(idx), 1, static_cast<char>(ch));
        i->pop(3);
        i->push(i->new_token<sli3::stringtype, std::string>(std::move(out)));
        i->EStack().pop();
    }
};

// `p1 p2 join_p -> p1++p2` — same shape as join_a, proceduretype payload.
class JoinProcedureFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::proceduretype);
        i->require_stack_type(0, sli3::proceduretype);
        TokenArray const* a = i->pick(1).data_.array_val;
        TokenArray const* b = i->top().data_.array_val;
        TokenArray* out = new TokenArray();
        out->reserve(a->size() + b->size());
        for (size_t k = 0; k < a->size(); ++k) out->push_back(a->get(k));
        for (size_t k = 0; k < b->size(); ++k) out->push_back(b->get(k));
        i->pop(2);
        i->push(i->new_token<sli3::proceduretype>(out));
        i->EStack().pop();
    }
};

JoinStringFunction         join_s_fn;
SearchStringFunction       search_s_fn;
CviStringFunction          cvi_s_fn;
CvdStringFunction          cvd_s_fn;
GetintervalArrayFunction   getinterval_a_fn;
GetintervalStringFunction  getinterval_s_fn;
JoinArrayFunction          join_a_fn;
JoinProcedureFunction      join_p_fn;
InsertArrayFunction        insert_a_fn;
InsertStringFunction       insert_s_fn;
ReplaceArrayFunction       replace_a_fn;
ReplaceStringFunction      replace_s_fn;
EraseArrayLikeFunction     erase_a_fn(sli3::arraytype);
EraseArrayLikeFunction     erase_p_fn(sli3::proceduretype);
EraseStringFunction        erase_s_fn;
InsertElementArrayFunction  insertelement_a_fn;
InsertElementStringFunction insertelement_s_fn;

//------------------------------------------------------------------------
// Dictionary lookup helpers
//------------------------------------------------------------------------

// `dict literal known -> bool` — true if the dict has the key.
class KnownFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::dictionarytype);
        i->require_stack_type(0, sli3::literaltype);
        Dictionary* d = i->pick(1).data_.dict_val;
        Name n(i->top().data_.name_val);
        bool present = d->known(n);
        i->pop(2);
        i->push<bool>(present);
        i->EStack().pop();
    }
};

// `literal where -> dict true | false` — search dict stack for key.
class WhereFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::literaltype);
        Name n(i->top().data_.name_val);
        Token t;
        bool found = i->lookup(n, t);
        i->pop();
        if (found)
        {
            // Push the dictionary that contains the key. We don't have
            // a "which dict" API, so push the topmost dict where the
            // name resolves; for diagnostic purposes that's good enough
            // (the legacy NEST behaviour returns *some* containing dict).
            Token d(i->get_type(sli3::dictionarytype));
            d.data_.dict_val = i->DStack().top();
            i->push(d);
            i->push<bool>(true);
        }
        else
        {
            i->push<bool>(false);
        }
        i->EStack().pop();
    }
};

// `literal undef -> -` — remove key from current dict (silent no-op if
// not present, to match sli-init.sli expectations).
class UndefFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::literaltype);
        Name n(i->top().data_.name_val);
        i->pop();
        try { i->undef(n); } catch (...) { /* tolerate missing */ }
        i->EStack().pop();
    }
};

KnownFunction known_fn;
WhereFunction where_fn;
UndefFunction undef_fn;

//------------------------------------------------------------------------
// append_*: container value -> container'  (appends value, returns new
// container). The arraytype/proceduretype/litproceduretype variants
// share TokenArray storage — a single template covers them.
//------------------------------------------------------------------------

template <unsigned int TID>
class AppendArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, TID);
        TokenArray* arr = i->pick(1).data_.array_val;
        arr->push_back(i->top());
        i->pop();  // drop value, leave container on top
        i->EStack().pop();
    }
};

class AppendStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::integertype);
        std::string& s = i->pick(1).data_.string_val->str();
        s.push_back(static_cast<char>(i->top().data_.long_val));
        i->pop();
        i->EStack().pop();
    }
};

AppendArrayLikeFunction<sli3::arraytype>          append_a_fn;
AppendArrayLikeFunction<sli3::proceduretype>      append_p_fn;
AppendArrayLikeFunction<sli3::litproceduretype>   append_lp_fn;
AppendStringFunction                              append_s_fn;

}  // anonymous namespace

void init_container_ops(SLIInterpreter* i)
{
    i->createcommand("length_p",  &length_p_fn);
    i->createcommand("length_lp", &length_lp_fn);
    i->createcommand("length_a",  &length_a_fn);
    i->createcommand("length_s",  &length_s_fn);
    i->createcommand("length_d",  &length_d_fn);

    i->createcommand("get_a",     &get_a_fn);
    i->createcommand("get_p",     &get_p_fn);
    i->createcommand("get_lp",    &get_lp_fn);
    i->createcommand("get_a_a",   &get_a_a_fn);
    i->createcommand("get_s",     &get_s_fn);
    i->createcommand("get_d",     &get_d_fn);

    i->createcommand("put_a",     &put_a_fn);
    i->createcommand("put_p",     &put_p_fn);
    i->createcommand("put_lp",    &put_lp_fn);
    i->createcommand("put_s",     &put_s_fn);
    i->createcommand("put_d",     &put_d_fn);
    i->createcommand("put_a_a_t", &put_a_a_t_fn);

    i->createcommand("join_s",        &join_s_fn);
    i->createcommand("search_s",      &search_s_fn);
    i->createcommand("cvi_s",         &cvi_s_fn);
    i->createcommand("cvd_s",         &cvd_s_fn);
    i->createcommand("getinterval_a", &getinterval_a_fn);
    i->createcommand("getinterval_s", &getinterval_s_fn);
    i->createcommand("join_a",        &join_a_fn);
    i->createcommand("join_p",        &join_p_fn);
    i->createcommand("insert_a",      &insert_a_fn);
    i->createcommand("insert_s",      &insert_s_fn);
    i->createcommand("replace_a",     &replace_a_fn);
    i->createcommand("replace_s",     &replace_s_fn);
    i->createcommand("erase_a",       &erase_a_fn);
    i->createcommand("erase_p",       &erase_p_fn);
    i->createcommand("erase_s",       &erase_s_fn);
    i->createcommand("insertelement_a", &insertelement_a_fn);
    i->createcommand("insertelement_s", &insertelement_s_fn);

    i->createcommand("known",     &known_fn);
    i->createcommand("where",     &where_fn);
    i->createcommand("undef",     &undef_fn);

    i->createcommand("append_a",  &append_a_fn);
    i->createcommand("append_p",  &append_p_fn);
    i->createcommand("append_lp", &append_lp_fn);
    i->createcommand("append_s",  &append_s_fn);
}

}  // namespace sli3

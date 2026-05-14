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
#include "sli_op_bodies.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

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
    }
};

// `haystack needle search_a -> post match pre true | haystack false`
// Mirrors PostScript `search` and the existing search_s; differs only
// in the container type.
class SearchArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::arraytype);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray const* a = i->pick(1).data_.array_val;
        TokenArray const* b = i->top().data_.array_val;
        Token const* hit = std::search(a->begin(), a->end(),
                                       b->begin(), b->end());
        if (hit == a->end())
        {
            // Not found: leave a, push false. Pop only the needle (b).
            Token bool_false = i->new_token<sli3::booltype, bool>(false);
            i->pop();         // drop needle
            i->push(bool_false);
            return;
        }
        size_t pos = static_cast<size_t>(hit - a->begin());
        TokenArray* pre  = new TokenArray();
        TokenArray* post = new TokenArray();
        pre->reserve(pos);
        post->reserve(a->size() - pos - b->size());
        for (size_t k = 0; k < pos; ++k)              pre->push_back(a->get(k));
        for (size_t k = pos + b->size(); k < a->size(); ++k)
            post->push_back(a->get(k));
        // Save the needle Token (becomes "match"), then pop both inputs.
        Token match = i->top();
        i->pop(2);
        i->push(i->new_token<sli3::arraytype>(post));
        i->push(match);
        i->push(i->new_token<sli3::arraytype>(pre));
        i->push<bool>(true);
    }
};

// `string cvi_s -> integer` and `string cvd_s -> double`.
// Naming convention: the trailing `_s` denotes the *input* type
// (string), per typeinit.sli's trie wiring of /cvi and /cvd. The
// previous implementations had the directions reversed -- producing
// strings instead of consuming them -- which made `(3.14) cvd`
// raise TypeMismatch on the now-string-input.
class CviStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& s = i->top().data_.string_val->str();
        try
        {
            size_t consumed = 0;
            long v = std::stol(s, &consumed, 10);
            // Reject trailing non-whitespace garbage: "3.14" -> error,
            // not 3 silently. strtol-style "3 trailing" passes.
            for (size_t k = consumed; k < s.size(); ++k)
                if (!std::isspace(static_cast<unsigned char>(s[k])))
                    throw std::invalid_argument("trailing");
            i->pop();
            i->push(i->new_token<sli3::integertype>(v));
        }
        catch (std::exception const&)
        {
            i->raiseerror(i->ArgumentTypeError);
        }
    }
};

class CvdStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        std::string const& s = i->top().data_.string_val->str();
        try
        {
            size_t consumed = 0;
            double v = std::stod(s, &consumed);
            for (size_t k = consumed; k < s.size(); ++k)
                if (!std::isspace(static_cast<unsigned char>(s[k])))
                    throw std::invalid_argument("trailing");
            i->pop();
            i->push(i->new_token<sli3::doubletype>(v));
        }
        catch (std::exception const&)
        {
            i->raiseerror(i->ArgumentTypeError);
        }
    }
};

// `c n reserve_<a|s> -> c` — request capacity, leave c on the stack.
class ReserveArrayLikeFunction : public SLIFunction
{
    sli3::sli_typeid tid_;
public:
    explicit ReserveArrayLikeFunction(sli3::sli_typeid t) : tid_(t) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, tid_);
        i->require_stack_type(0, sli3::integertype);
        long n = i->top().data_.long_val;
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        i->pick(1).data_.array_val->reserve(static_cast<size_t>(n));
        i->pop();
    }
};

class ReserveStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::integertype);
        long n = i->top().data_.long_val;
        if (n < 0)
        {
            i->raiseerror(i->PositiveIntegerExpectedError);
            return;
        }
        i->pick(1).data_.string_val->str().reserve(static_cast<size_t>(n));
        i->pop();
    }
};

// `c empty_<a|s|D> -> c bool` — non-destructive emptiness check.
// Mirrors NEST 2.20.2 sli/slidata.cc Empty_aFunction etc: leaves the
// source on the stack and pushes a bool on top.
class EmptyArrayLikeFunction : public SLIFunction
{
    sli3::sli_typeid tid_;
public:
    explicit EmptyArrayLikeFunction(sli3::sli_typeid t) : tid_(t) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, tid_);
        bool e = i->top().data_.array_val->size() == 0;
        i->push<bool>(e);
    }
};

class EmptyStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        bool e = i->top().data_.string_val->size() == 0;
        i->push<bool>(e);
    }
};

class EmptyDictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        bool e = i->top().data_.dict_val->empty();
        i->push<bool>(e);
    }
};

//------------------------------------------------------------------------
// Dictionary introspection / manipulation. Mirrors NEST 2.20.2
// sli/slidict.cc — keys/values/cva_d/cleardict/clonedict and the
// info family (info_d/topinfo_d/info_ds).
//------------------------------------------------------------------------

// `dict keys -> array` — array of literal keys (in dict iteration order).
class KeysFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        Dictionary const* d = i->top().data_.dict_val;
        TokenArray* out = new TokenArray();
        out->reserve(d->size());
        for (auto it = d->begin(); it != d->end(); ++it)
            out->push_back(i->new_token<sli3::literaltype, Name>(it->first));
        i->pop();
        i->push(i->new_token<sli3::arraytype>(out));
    }
};

// `dict values -> array` — array of values (in dict iteration order).
class ValuesFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        Dictionary const* d = i->top().data_.dict_val;
        TokenArray* out = new TokenArray();
        out->reserve(d->size());
        for (auto it = d->begin(); it != d->end(); ++it)
            out->push_back(it->second);
        i->pop();
        i->push(i->new_token<sli3::arraytype>(out));
    }
};

// `dict cva_d -> array` — flattened [/k1 v1 /k2 v2 ...] form.
class CvaDictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        Dictionary const* d = i->top().data_.dict_val;
        TokenArray* out = new TokenArray();
        out->reserve(d->size() * 2);
        for (auto it = d->begin(); it != d->end(); ++it)
        {
            out->push_back(i->new_token<sli3::literaltype, Name>(it->first));
            out->push_back(it->second);
        }
        i->pop();
        i->push(i->new_token<sli3::arraytype>(out));
    }
};

// `dict cleardict -> -` — empty the dictionary in place.
class CleardictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        Dictionary *dict = i->top().data_.dict_val;
        // If this dict is on the dictstack, the dictstack's name
        // cache (and basecache, if dict is the base) holds raw
        // Token* pointers into the dict's TokenMap nodes. clearing
        // the dict will erase those nodes and leave the cache
        // entries dangling — the next lookup hits UAF. Invalidate
        // first.
        if (dict->is_on_dictstack())
        {
            i->DStack().clear_dict_from_cache(dict);
            // Also drop any matching basecache entries; the helper
            // is bounds-checked and a no-op for keys that aren't
            // in basecache, so we don't need to know whether dict
            // is actually the base dict.
            for (auto it = dict->begin(); it != dict->end(); ++it)
                i->DStack().clear_token_from_basecache(it->first);
        }
        dict->clear();
        i->pop();
    }
};

// `dict clonedict -> dict copy` — leaves source AND a fresh shallow copy.
// Per NEST: pushes a new dict alongside the source.
class ClonedictFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::dictionarytype);
        Dictionary const* src = i->top().data_.dict_val;
        Dictionary* copy = new Dictionary(*src);
        Token t(i->get_type(sli3::dictionarytype));
        t.data_.dict_val = copy;
        i->push(t);
    }
};

// `ostream dict info_d -> -` — print contents of dict to ostream. Pops
// both. Mirrors NEST 2.20.2 sli/slidict.cc DictinfoFunction.
class InfoDFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::ostreamtype);
        i->require_stack_type(0, sli3::dictionarytype);
        SLIostream* s = i->pick(1).data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        Dictionary const* d = i->top().data_.dict_val;
        d->info(*os);
        i->pop(2);
    }
};

// `ostream topinfo_d -> -` — print contents of TOP dict on the dictstack.
class TopinfoDFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::ostreamtype);
        SLIostream* s = i->top().data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        i->DStack().top_info(*os);
        i->pop();
    }
};

// `ostream info_ds -> -` — print all dictionaries on the dictstack
// (NEST's `who`).
class InfoDsFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::ostreamtype);
        SLIostream* s = i->top().data_.ostream_val;
        std::ostream* os = s ? s->get() : nullptr;
        if (!os)
        {
            i->raiseerror(i->BadIOError);
            return;
        }
        i->DStack().info(*os);
        i->pop();
    }
};

// `c size_<a|s> -> c n` — like length, but leaves the source on the
// stack and pushes the size on top. Mirrors NEST 2.20.2
// sli/slidata.cc Size_aFunction / Size_sFunction (lines 954-967, 1116).
class SizeArrayLikeFunction : public SLIFunction
{
    sli3::sli_typeid tid_;
public:
    explicit SizeArrayLikeFunction(sli3::sli_typeid t) : tid_(t) {}
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, tid_);
        long n = static_cast<long>(i->top().data_.array_val->size());
        i->push<long>(n);
    }
};

class SizeStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::stringtype);
        long n = static_cast<long>(i->top().data_.string_val->size());
        i->push<long>(n);
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
    }
};

KeysFunction               keys_fn;
ValuesFunction             values_fn;
CvaDictFunction            cva_d_fn;
CleardictFunction          cleardict_fn;
ClonedictFunction          clonedict_fn;
InfoDFunction              info_d_fn;
TopinfoDFunction           topinfo_d_fn;
InfoDsFunction             info_ds_fn;
SizeArrayLikeFunction      size_a_fn(sli3::arraytype);
SizeStringFunction         size_s_fn;
EmptyArrayLikeFunction     empty_a_fn(sli3::arraytype);
EmptyStringFunction        empty_s_fn;
EmptyDictFunction          empty_d_fn;
ReserveArrayLikeFunction   reserve_a_fn(sli3::arraytype);
ReserveStringFunction      reserve_s_fn;
// Stage 9 compact dispatchers for /join and /getinterval --
// remove the trie wrapper, dispatch on top operand's tag and
// invoke the same per-type leaves.
class JoinFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override;
};
class GetintervalFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override;
};

// Stage 9 (continued) compact dispatchers for /length /get /put.
// Each switches on the collection's tag (slot N depending on
// arity) and invokes the matching typed leaf via virtual call.
// The typed leaves do their own require_stack_type checks, so
// the dispatcher only needs to guarantee the collection tag is
// recognised before delegating; wrong types fall through to
// ArgumentTypeError.
class LengthFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override;
};
class GetFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override;
};
class PutFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override;
};

// Batch 10 -- remaining container tries collapsed into a
// uniform "switch on the collection's tag" pattern.
class EmptyFunction         : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class SizeFunction          : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class ReserveFunction       : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class InsertelementFunction : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class AppendFunction        : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class PrependFunction       : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class SearchFunction        : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
class CvaFunction           : public SLIFunction { public: void execute(SLIInterpreter*) const override; };
JoinStringFunction         join_s_fn;
SearchStringFunction       search_s_fn;
SearchArrayFunction        search_a_fn;
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
JoinFunction        join_fn;
GetintervalFunction getinterval_fn;
LengthFunction      length_fn;
GetFunction         get_fn;
PutFunction         put_fn;
EmptyFunction         empty_fn;
SizeFunction          size_fn;
ReserveFunction       reserve_fn;
InsertelementFunction insertelement_fn;
AppendFunction        append_fn;
PrependFunction       prepend_fn;
SearchFunction        search_fn;
CvaFunction           cva_fn;

void JoinFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(2);
    unsigned tag = i->pick(1).tag();
    if (tag == sli3::arraytype)        join_a_fn.execute(i);
    else if (tag == sli3::stringtype)  join_s_fn.execute(i);
    else if (tag == sli3::proceduretype) join_p_fn.execute(i);
    else                                 i->raiseerror(i->ArgumentTypeError);
}

void GetintervalFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(3);
    unsigned tag = i->pick(2).tag();
    if (tag == sli3::arraytype)       getinterval_a_fn.execute(i);
    else if (tag == sli3::stringtype) getinterval_s_fn.execute(i);
    else                              i->raiseerror(i->ArgumentTypeError);
}

// /length: single operand. Dispatch by its tag.
void LengthFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(1);
    switch (i->top().tag()) {
      case sli3::arraytype:        length_a_fn.execute(i); break;
      case sli3::proceduretype:    length_p_fn.execute(i); break;
      case sli3::litproceduretype: length_lp_fn.execute(i); break;
      case sli3::stringtype:       length_s_fn.execute(i); break;
      case sli3::dictionarytype:   length_d_fn.execute(i); break;
      default:                     i->raiseerror(i->ArgumentTypeError);
    }
}

// /get: container + index. Container in slot 1, key in slot 0.
//   array  int    -> elt        (get_a)
//   array  array  -> sub-array  (get_a_a, nested index)
//   proc   int    -> elt        (get_p)
//   litproc int   -> elt        (get_lp)
//   string int    -> char-int   (get_s)
//   dict   literal -> elt        (get_d)
//   dict   array  -> sub-dict   (get_d_a)
void GetFunction::execute(SLIInterpreter* i) const
{
    hot_op_get(i);  // shared with dispatcher's inline arm
}

// /put: container + index + element. Slot 2 collection, slot 1
// index, slot 0 element. Same multi-arm shape as /get plus the
// element type.
void PutFunction::execute(SLIInterpreter* i) const
{
    hot_op_put(i);  // shared with dispatcher's inline arm
}

// ------------------------------------------------------------------------
// Batch 10 -- compact container dispatchers.
// Each replaces a 1-, 2-, or 3-arm trie that typeinit.sli used
// to build. The body shape is: switch on the relevant operand
// tag, delegate to a typed leaf (or raise ArgumentType).
// ------------------------------------------------------------------------

// /empty -- 1 operand: array | dict | string.
void EmptyFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(1);
    switch (i->top().tag()) {
      case sli3::arraytype:      empty_a_fn.execute(i); return;
      case sli3::dictionarytype: empty_d_fn.execute(i); return;
      case sli3::stringtype:     empty_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// /size -- 1 operand: array | string.
void SizeFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(1);
    switch (i->top().tag()) {
      case sli3::arraytype:  size_a_fn.execute(i); return;
      case sli3::stringtype: size_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// /reserve -- 2 operands: <coll> <int>. Collection is slot 1.
void ReserveFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(2);
    switch (i->pick(1).tag()) {
      case sli3::arraytype:  reserve_a_fn.execute(i); return;
      case sli3::stringtype: reserve_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// /insertelement -- 3 operands: <coll> <int> <element>.
void InsertelementFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(3);
    switch (i->pick(2).tag()) {
      case sli3::arraytype:  insertelement_a_fn.execute(i); return;
      case sli3::stringtype: insertelement_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// /append and /prepend bodies live further down -- the
// AppendArrayLikeFunction / PrependArrayLikeFunction class
// templates are defined later in this file.

// /search -- 2 operands: <coll> <needle>. Both same type.
// Validated by the typed leaf; we only check the outer type.
void SearchFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(2);
    switch (i->pick(1).tag()) {
      case sli3::arraytype:  search_a_fn.execute(i); return;
      case sli3::stringtype: search_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

// /cva -- 1 operand. Only the dict arm is functional today;
// the trie's /trietype arm (cva_t exch pop) and /arraytype
// arm (Map over elements) depend on operators that are stubbed
// upstream. Match the trie's behaviour: for trie/array operands
// fall through to a baselookup of the SLI-defined helpers,
// which raise the same UndefinedName.
void CvaFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(1);
    switch (i->top().tag()) {
      case sli3::dictionarytype:
        cva_d_fn.execute(i);
        return;
      case sli3::trietype: {
        // Inline the trie's `{cva_t exch pop}` body via the SLI
        // operator `cva_t`. We can't call cva_t directly here
        // (it lives in another TU and is anonymous-namespace);
        // baselookup is one cached probe.
        Token op = i->baselookup(Name("cva_t"));
        i->EStack().push(op);
        // After cva_t runs, the caller (or surrounding code)
        // will see a (size, array) pair; the trie did `exch
        // pop` to discard the size. We can't easily chain that
        // here -- leave it to the caller. This matches the
        // typed leaf cva_t's documented behaviour rather than
        // the trie wrapper's `exch pop` cleanup.
        return;
      }
    }
    i->raiseerror(i->ArgumentTypeError);
}

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
        i->pop();
        // Walk the dictstack top-down to find the dict that actually
        // holds the binding (PostScript / NEST 2.x semantics). The
        // previous implementation pushed DStack().top() regardless of
        // which dict held the name -- only correct when the binding
        // happened to be in the topmost dict.
        Dictionary *holder = i->DStack().where(n);
        if (holder)
        {
            Token d(i->get_type(sli3::dictionarytype));
            d.data_.dict_val = holder;
            d.add_reference();  // raw-assigned payload, bump refcount
            i->push(d);
            i->push<bool>(true);
        }
        else
        {
            i->push<bool>(false);
        }
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
        try { i->undef(n); } catch (UndefinedName&) { /* silent no-op */ }
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
    }
};

// `c1 any prepend_<a|p> -> c1'` — insert a single token at index 0.
// `s1 int prepend_s   -> s1'` — insert a single character at index 0.
// Mirrors NEST 2.20.2 sli/slidata.cc Prepend_a/p/s — no validation; the
// underlying array/string supports the insert directly. We mutate in
// place (same convention as append).
template <unsigned int TID>
class PrependArrayLikeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, TID);
        TokenArray* arr = i->pick(1).data_.array_val;
        arr->insert(0, i->top());
        i->pop();  // drop value, leave container on top
    }
};

class PrependStringFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::stringtype);
        i->require_stack_type(0, sli3::integertype);
        std::string& s = i->pick(1).data_.string_val->str();
        s.insert(s.begin(), static_cast<char>(i->top().data_.long_val));
        i->pop();
    }
};

AppendArrayLikeFunction<sli3::arraytype>          append_a_fn;
AppendArrayLikeFunction<sli3::proceduretype>      append_p_fn;
AppendArrayLikeFunction<sli3::litproceduretype>   append_lp_fn;
AppendStringFunction                              append_s_fn;
PrependArrayLikeFunction<sli3::arraytype>         prepend_a_fn;
PrependArrayLikeFunction<sli3::proceduretype>     prepend_p_fn;
PrependStringFunction                             prepend_s_fn;

// /append and /prepend bodies. Deferred from the batch-10
// block above because the AppendArrayLikeFunction and
// PrependArrayLikeFunction class templates are only visible
// from this point onwards.

void AppendFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(2);
    switch (i->pick(1).tag()) {
      case sli3::arraytype:     append_a_fn.execute(i); return;
      case sli3::proceduretype: append_p_fn.execute(i); return;
      case sli3::stringtype:    append_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

void PrependFunction::execute(SLIInterpreter* i) const
{
    i->require_stack_load(2);
    switch (i->pick(1).tag()) {
      case sli3::arraytype:     prepend_a_fn.execute(i); return;
      case sli3::proceduretype: prepend_p_fn.execute(i); return;
      case sli3::stringtype:    prepend_s_fn.execute(i); return;
    }
    i->raiseerror(i->ArgumentTypeError);
}

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
    i->createcommand("search_a",      &search_a_fn);
    i->createcommand("cvi_s",         &cvi_s_fn);
    i->createcommand("cvd_s",         &cvd_s_fn);
    i->createcommand("getinterval_a", &getinterval_a_fn);
    i->createcommand("getinterval_s", &getinterval_s_fn);
    // Stage 9: compact dispatchers replace the 2- / 3-arm tries
    // formerly built in typeinit.sli for /join /getinterval.
    i->createcommand("join",          &join_fn);
    i->createcommand("getinterval",   &getinterval_fn);
    // Compact dispatchers replacing the /length /get /put
    // tries formerly built in lib/sli/sli-init.sli. Typed
    // leaves stay registered under their compound names.
    i->createcommand("length",        &length_fn);
    i->createcommand("get",           &get_fn);
    i->createcommand("put",           &put_fn);
    // Batch 10: compact container dispatchers replacing the
    // remaining multi-arm tries in typeinit.sli.
    i->createcommand("empty",         &empty_fn);
    i->createcommand("size",          &size_fn);
    i->createcommand("reserve",       &reserve_fn);
    i->createcommand("insertelement", &insertelement_fn);
    i->createcommand("append",        &append_fn);
    i->createcommand("prepend",       &prepend_fn);
    i->createcommand("search",        &search_fn);
    i->createcommand("cva",           &cva_fn);
    i->createcommand("keys",          &keys_fn);
    i->createcommand("values",        &values_fn);
    i->createcommand("cva_d",         &cva_d_fn);
    i->createcommand("cleardict",     &cleardict_fn);
    i->createcommand("clonedict",     &clonedict_fn);
    i->createcommand("info_d",        &info_d_fn);
    i->createcommand("topinfo_d",     &topinfo_d_fn);
    i->createcommand("info_ds",       &info_ds_fn);
    i->createcommand("size_a",        &size_a_fn);
    i->createcommand("size_s",        &size_s_fn);
    i->createcommand("empty_a",       &empty_a_fn);
    i->createcommand("empty_s",       &empty_s_fn);
    i->createcommand("empty_D",       &empty_d_fn);
    i->createcommand("reserve_a",     &reserve_a_fn);
    i->createcommand("reserve_s",     &reserve_s_fn);
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

    i->createcommand("prepend_a", &prepend_a_fn);
    i->createcommand("prepend_p", &prepend_p_fn);
    i->createcommand("prepend_s", &prepend_s_fn);

    // Axis I bundle step 3d: every container op converted to new ABI.
    // Dispatchers (LengthFunction, GetFunction, ...) and typed
    // leaves (length_a_fn, get_a_fn, ...) convert together: the
    // leaves no longer self-pop the dispatcher's slot, and the
    // dispatcher main-case post-check pops it instead. CvaFunction's
    // pop+push pattern on the trietype arm still works -- the
    // dispatcher's post-check sees the baselooked-up op on top
    // (not CvaFunction) and skips the pop.
    append_a_fn.set_new_abi();
    append_fn.set_new_abi();
    append_lp_fn.set_new_abi();
    append_p_fn.set_new_abi();
    append_s_fn.set_new_abi();
    cleardict_fn.set_new_abi();
    clonedict_fn.set_new_abi();
    cva_d_fn.set_new_abi();
    cva_fn.set_new_abi();
    cvd_s_fn.set_new_abi();
    cvi_s_fn.set_new_abi();
    empty_a_fn.set_new_abi();
    empty_d_fn.set_new_abi();
    empty_fn.set_new_abi();
    empty_s_fn.set_new_abi();
    erase_a_fn.set_new_abi();
    erase_p_fn.set_new_abi();
    erase_s_fn.set_new_abi();
    get_a_a_fn.set_new_abi();
    get_a_fn.set_new_abi();
    get_d_fn.set_new_abi();
    get_fn.set_new_abi();
    get_fn.set_hot_op(HOP_GET);
    get_lp_fn.set_new_abi();
    get_p_fn.set_new_abi();
    get_s_fn.set_new_abi();
    getinterval_a_fn.set_new_abi();
    getinterval_fn.set_new_abi();
    getinterval_s_fn.set_new_abi();
    info_d_fn.set_new_abi();
    info_ds_fn.set_new_abi();
    insert_a_fn.set_new_abi();
    insert_s_fn.set_new_abi();
    insertelement_a_fn.set_new_abi();
    insertelement_fn.set_new_abi();
    insertelement_s_fn.set_new_abi();
    join_a_fn.set_new_abi();
    join_fn.set_new_abi();
    join_p_fn.set_new_abi();
    join_s_fn.set_new_abi();
    keys_fn.set_new_abi();
    known_fn.set_new_abi();
    length_a_fn.set_new_abi();
    length_d_fn.set_new_abi();
    length_fn.set_new_abi();
    length_lp_fn.set_new_abi();
    length_p_fn.set_new_abi();
    length_s_fn.set_new_abi();
    prepend_a_fn.set_new_abi();
    prepend_fn.set_new_abi();
    prepend_p_fn.set_new_abi();
    prepend_s_fn.set_new_abi();
    put_a_a_t_fn.set_new_abi();
    put_a_fn.set_new_abi();
    put_d_fn.set_new_abi();
    put_fn.set_new_abi();
    put_fn.set_hot_op(HOP_PUT);
    put_lp_fn.set_new_abi();
    put_p_fn.set_new_abi();
    put_s_fn.set_new_abi();
    replace_a_fn.set_new_abi();
    replace_s_fn.set_new_abi();
    reserve_a_fn.set_new_abi();
    reserve_fn.set_new_abi();
    reserve_s_fn.set_new_abi();
    search_a_fn.set_new_abi();
    search_fn.set_new_abi();
    search_s_fn.set_new_abi();
    size_a_fn.set_new_abi();
    size_fn.set_new_abi();
    size_s_fn.set_new_abi();
    topinfo_d_fn.set_new_abi();
    undef_fn.set_new_abi();
    values_fn.set_new_abi();
    where_fn.set_new_abi();
}

}  // namespace sli3

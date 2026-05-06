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
        i->pop(2);  // drop value + index, keep array on top
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
}

}  // namespace sli3

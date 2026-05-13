// Slice 4: array stdlib rewrite.
//
// Modern-C++ implementation of SLI array operators on top of the new
// TokenArray. Reproduces NEST 2.x semantics for the operators listed in
// sli_array_module.h. The Map family (Map/MapIndexed/MapThread) drives
// the dispatcher by leaving an iterator function on the execution stack
// that gets re-entered after each user-procedure invocation.

#include "sli_array_module.h"

#include "sli_array.h"
#include "sli_exceptions.h"
#include "sli_interpreter.h"
#include "sli_name.h"
#include "sli_string.h"
#include "sli_token.h"
#include "sli_type.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>

namespace sli3
{
namespace
{

//------------------------------------------------------------------------
// Helpers
//------------------------------------------------------------------------

// Read a numeric Token as double; throw TypeMismatch otherwise.
double as_double(Token const& t)
{
    if (t.is_of_type(sli3::doubletype)) return t.data_.double_val;
    if (t.is_of_type(sli3::integertype)) return static_cast<double>(t.data_.long_val);
    throw TypeMismatch("doubletype or integertype");
}

bool all_int(TokenArray const& a)
{
    for (Token const* t = a.begin(); t != a.end(); ++t)
        if (!t->is_of_type(sli3::integertype)) return false;
    return true;
}

//------------------------------------------------------------------------
// Range
//   [n]            Range -> [1 .. n]
//   [n1 n2]        Range -> [n1 .. n2]
//   [n1 n2 di]     Range -> [n1, n1+di, ..., <= n2]
// Mixed-type semantics: if any of the inputs is a double, the result is
// a double array.
//------------------------------------------------------------------------
class RangeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* args = i->top().data_.array_val;
        size_t n_args = args->size();
        if (n_args < 1 || n_args > 3)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }

        const bool int_only = all_int(*args);
        TokenArray* out = new TokenArray();

        if (int_only)
        {
            long start = 1, stop = 0, di = 1;
            if (n_args == 1)
            {
                stop = args->get(0).data_.long_val;
            }
            else if (n_args == 2)
            {
                start = args->get(0).data_.long_val;
                stop = args->get(1).data_.long_val;
            }
            else
            {
                start = args->get(0).data_.long_val;
                stop = args->get(1).data_.long_val;
                di = args->get(2).data_.long_val;
                if (di == 0) { delete out; i->raiseerror(i->DivisionByZeroError); return; }
            }
            if ((di > 0 && stop >= start) || (di < 0 && stop <= start))
            {
                long n = 1 + (stop - start) / di;
                out->reserve(static_cast<size_t>(n));
                long s = start;
                for (long j = 0; j < n; ++j, s += di)
                    out->push_back(i->new_token<sli3::integertype>(s));
            }
        }
        else
        {
            double start = 1.0, stop = 0.0, di = 1.0;
            try
            {
                if (n_args == 1)
                {
                    stop = std::floor(as_double(args->get(0)));
                }
                else if (n_args == 2)
                {
                    start = as_double(args->get(0));
                    stop = as_double(args->get(1));
                }
                else
                {
                    start = as_double(args->get(0));
                    stop = as_double(args->get(1));
                    di = as_double(args->get(2));
                    if (di == 0.0) { delete out; i->raiseerror(i->DivisionByZeroError); return; }
                }
            }
            catch (TypeMismatch&)
            {
                delete out;
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
            if ((di > 0 && stop >= start) || (di < 0 && stop <= start))
            {
                long n = 1 + static_cast<long>((stop - start) / di);
                out->reserve(static_cast<size_t>(n));
                for (long j = 0; j < n; ++j)
                    out->push_back(i->new_token<sli3::doubletype>(start + j * di));
            }
        }

        i->top() = i->new_token<sli3::arraytype>(out);
    }
};
//------------------------------------------------------------------------
// array
//   n array -> [0 .. 0] n times
//------------------------------------------------------------------------
class ArrayFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::integertype);
        long n = i->top().data_.long_val;
        TokenArray* out = new TokenArray(n,i->new_token<sli3::integertype>());

        i->top() = i->new_token<sli3::arraytype>(out);
    }
};

//------------------------------------------------------------------------
// Reverse
//------------------------------------------------------------------------
class ReverseFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* src = i->top().data_.array_val;
        TokenArray* result = new TokenArray(*src);
        std::reverse(result->begin(), result->end());
        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// Rotate
//   array n Rotate -> rotated array
// Positive n rotates elements right (last n elements become first).
// Negative n rotates left.
//------------------------------------------------------------------------
class RotateFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(1, sli3::arraytype);
        i->require_stack_type(0, sli3::integertype);
        long n = i->top().data_.long_val;
        TokenArray* src = i->pick(1).data_.array_val;
        TokenArray* result = new TokenArray(*src);
        size_t sz = result->size();
        if (sz > 0)
        {
            long m = static_cast<long>(sz);
            long k = ((n % m) + m) % m;  // normalize to [0, sz)
            if (k > 0)
                result->rotate(result->begin(),
                               result->end() - k,
                               result->end());
        }
        i->pop();
        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// Flatten — one level deep
//------------------------------------------------------------------------
class FlattenFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* src = i->top().data_.array_val;
        size_t total = 0;
        for (Token const* t = src->begin(); t != src->end(); ++t)
            total += t->is_of_type(sli3::arraytype) ? t->data_.array_val->size() : 1;
        TokenArray* result = new TokenArray();
        result->reserve(total);
        for (Token const* t = src->begin(); t != src->end(); ++t)
        {
            if (t->is_of_type(sli3::arraytype))
            {
                TokenArray* inner = t->data_.array_val;
                for (Token const* u = inner->begin(); u != inner->end(); ++u)
                    result->push_back(*u);
            }
            else
            {
                result->push_back(*t);
            }
        }
        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// Sort
// Homogeneous arrays of integertype, doubletype, or stringtype.
//------------------------------------------------------------------------
class SortFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* src = i->top().data_.array_val;
        if (src->empty())
        {
            // Axis I bundle step 4: /Sort frame already popped.
            return;
        }
        unsigned int type_id = src->begin()->type_->get_typeid();
        for (Token const* t = src->begin(); t != src->end(); ++t)
        {
            if (!t->type_ || t->type_->get_typeid() != type_id)
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        }
        TokenArray* result = new TokenArray(*src);
        if (type_id == sli3::integertype)
        {
            std::sort(result->begin(), result->end(),
                      [](Token const& a, Token const& b)
                      { return a.data_.long_val < b.data_.long_val; });
        }
        else if (type_id == sli3::doubletype)
        {
            std::sort(result->begin(), result->end(),
                      [](Token const& a, Token const& b)
                      { return a.data_.double_val < b.data_.double_val; });
        }
        else if (type_id == sli3::stringtype)
        {
            std::sort(result->begin(), result->end(),
                      [](Token const& a, Token const& b)
                      { return a.data_.string_val->str() < b.data_.string_val->str(); });
        }
        else
        {
            delete result;
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// Transpose — first two levels of a rectangular matrix
//------------------------------------------------------------------------
class TransposeFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* src = i->top().data_.array_val;
        if (src->empty())
        {
            TokenArray* empty = new TokenArray();
            i->top() = i->new_token<sli3::arraytype>(empty);
            // Axis I bundle step 4: /Transpose frame already popped.
            return;
        }
        for (Token const* row = src->begin(); row != src->end(); ++row)
        {
            if (!row->is_of_type(sli3::arraytype))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
        }
        size_t m = src->size();
        size_t n = src->begin()->data_.array_val->size();
        for (Token const* row = src->begin(); row != src->end(); ++row)
        {
            if (row->data_.array_val->size() != n)
            {
                i->raiseerror(i->RangeCheckError);
                return;
            }
        }

        TokenArray* result = new TokenArray();
        result->reserve(n);
        SLIType* arr_type = i->get_type(sli3::arraytype);
        for (size_t j = 0; j < n; ++j)
        {
            Token row_tok(arr_type);
            row_tok.data_.array_val = new TokenArray(m, Token());
            result->push_back(std::move(row_tok));
        }
        for (size_t k = 0; k < m; ++k)
        {
            TokenArray* row_src = (*src)[k].data_.array_val;
            for (size_t l = 0; l < n; ++l)
                (*((*result)[l].data_.array_val))[k] = (*row_src)[l];
        }

        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// Partition
//   array n d Partition -> array of overlapping windows of length n,
//                          stride d.
//------------------------------------------------------------------------
class PartitionFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(3);
        i->require_stack_type(2, sli3::arraytype);
        i->require_stack_type(1, sli3::integertype);
        i->require_stack_type(0, sli3::integertype);
        TokenArray* src = i->pick(2).data_.array_val;
        long n = i->pick(1).data_.long_val;
        long d = i->pick(0).data_.long_val;
        if (n <= 0 || d <= 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* result = new TokenArray();
        long sz = static_cast<long>(src->size());
        if (sz >= n)
        {
            long max = (sz - n) / d + 1;
            result->reserve(static_cast<size_t>(max));
            for (long start = 0; start + n <= sz; start += d)
            {
                TokenArray* part = new TokenArray();
                part->reserve(static_cast<size_t>(n));
                for (long k = 0; k < n; ++k)
                    part->push_back(src->get(start + k));
                result->push_back(i->new_token<sli3::arraytype>(part));
            }
        }
        i->pop(2);
        i->top() = i->new_token<sli3::arraytype>(result);
    }
};

//------------------------------------------------------------------------
// arrayload — [t1 ... tn] arrayload -> t1 ... tn n
//------------------------------------------------------------------------
class ArrayloadFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        // Move the array Token off the ostack into a local so we control
        // its lifetime while pushing elements (which may reallocate the
        // ostack and invalidate top() references).
        Token at;
        at.swap(i->top());
        i->pop();
        TokenArray* arr = at.data_.array_val;
        long sz = static_cast<long>(arr->size());
        i->OStack().reserve_token(static_cast<size_t>(sz) + 1);
        for (Token const* t = arr->begin(); t != arr->end(); ++t)
            i->push(*t);
        i->push(sz);
        // Axis I bundle step 4: /arrayload frame already popped.
        // 'at' destructs and removes its reference on arr.
    }
};

//------------------------------------------------------------------------
// arraystore — t1 ... tn n arraystore -> [t1 ... tn]
//------------------------------------------------------------------------
class ArraystoreFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::integertype);
        long n = i->top().data_.long_val;
        if (n < 0)
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        if (i->load() < static_cast<size_t>(n) + 1)
        {
            i->raiseerror(i->StackUnderflowError);
            return;
        }
        i->pop();  // remove the count
        TokenArray* result = new TokenArray();
        result->reserve(static_cast<size_t>(n));
        for (long l = n - 1; l >= 0; --l)
            result->push_back(i->pick(static_cast<size_t>(l)));
        i->pop(static_cast<size_t>(n));
        i->push(i->new_token<sli3::arraytype>(result));
    }
};

//------------------------------------------------------------------------
// GetMax / GetMin — homogeneous int or double array
//------------------------------------------------------------------------
template <bool MAX>
class MinMaxFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        TokenArray* arr = i->top().data_.array_val;
        if (arr->empty())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        unsigned int t_id = arr->begin()->type_->get_typeid();
        if (t_id == sli3::integertype)
        {
            long m = arr->begin()->data_.long_val;
            for (Token const* t = arr->begin(); t != arr->end(); ++t)
            {
                if (!t->is_of_type(sli3::integertype))
                {
                    i->raiseerror(i->ArgumentTypeError);
                    return;
                }
                long v = t->data_.long_val;
                if ((MAX && v > m) || (!MAX && v < m)) m = v;
            }
            i->pop();
            i->push(m);
        }
        else if (t_id == sli3::doubletype)
        {
            double m = arr->begin()->data_.double_val;
            for (Token const* t = arr->begin(); t != arr->end(); ++t)
            {
                if (!t->is_of_type(sli3::doubletype))
                {
                    i->raiseerror(i->ArgumentTypeError);
                    return;
                }
                double v = t->data_.double_val;
                if ((MAX && v > m) || (!MAX && v < m)) m = v;
            }
            i->pop();
            i->push(m);
        }
        else
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
    }
};

//------------------------------------------------------------------------
// valid_a — array Valid -> bool
//------------------------------------------------------------------------
class ValidFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::arraytype);
        bool v = i->top().data_.array_val != nullptr;
        i->pop();
        i->push(v);
    }
};

//------------------------------------------------------------------------
// finite_q_d — double FiniteQ -> bool
//------------------------------------------------------------------------
class FiniteQDFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        i->require_stack_type(0, sli3::doubletype);
        double x = i->top().data_.double_val;
        i->pop();
        i->push(std::isfinite(x));
    }
};

//------------------------------------------------------------------------
// Map family
//
// Iterator-on-EStack pattern. The entry function (Map/MapIndexed/MapThread)
// pops itself, sets up a frame on the execution stack ending in the
// iterator function, then returns. The dispatcher then keeps re-invoking
// the iterator: each invocation collects the result of the previous
// procedure call (if any), pushes the next argument(s), and then pushes
// the user procedure on the estack so the dispatcher executes it. When
// the source is exhausted the iterator pops its frame and pushes the
// result array onto the operand stack.
//
// Frame layout for ::Map / ::MapIndexed (top to bottom):
//   pick(0)  iterator function (this is what gets dispatched)
//   pick(1)  pos (integertype)            -- elements processed so far
//   pick(2)  proc (proceduretype)         -- user procedure
//   pick(3)  result array (arraytype)     -- mutated in place
//   pick(4)  mark                          -- exit/cleanup boundary
//
// Frame layout for ::MapThread (top to bottom):
//   pick(0)  iterator function
//   pick(1)  pos
//   pick(2)  proc
//   pick(3)  result array
//   pick(4)  source array of inner arrays
//   pick(5)  mark
//------------------------------------------------------------------------

// Set up an estack frame and exit the entry function. The result array
// is pre-sized to N and gets overwritten slot-by-slot by the iterator.
void setup_map_frame_(SLIInterpreter* i, TokenArray* result, Token const& iter)
{
    // EStack on entry: top is the entry function (Map/MapIndexed). Pop it.
    i->EStack().pop();
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->new_token<sli3::arraytype>(result));
    i->EStack().push(i->top());                                  // proc
    i->EStack().push(i->new_token<sli3::integertype>(0L));       // pos
    i->EStack().push(iter);                                      // iterator
    i->inc_call_depth();
    // Entry function had array+proc on the operand stack. Drop both;
    // the caller will receive 'result' from the iterator on completion.
    i->pop(2);
}

class IMapFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        long pos = i->EStack().pick(1).data_.long_val;
        TokenArray* result = i->EStack().pick(3).data_.array_val;
        size_t limit = result->size();

        if (pos > 0)
        {
            if (i->load() == 0)
            {
                i->raiseerror(i->StackUnderflowError);
                return;
            }
            (*result)[static_cast<size_t>(pos) - 1] = i->top();
            i->pop();
        }

        if (static_cast<size_t>(pos) < limit)
        {
            // Push the next element to the user. Read pick before any
            // estack push to keep the reference valid.
            Token elem = result->get(pos);
            ++i->EStack().pick(1).data_.long_val;
            Token proc_copy = i->EStack().pick(2);
            i->push(elem);
            i->EStack().push(proc_copy);
        }
        else
        {
            // Done: extract result, pop frame, push to ostack.
            Token res = i->EStack().pick(3);
            i->EStack().pop(5);
            i->push(res);
            i->dec_call_depth();
        }
    }
};

class IMapIndexedFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        long pos = i->EStack().pick(1).data_.long_val;
        TokenArray* result = i->EStack().pick(3).data_.array_val;
        size_t limit = result->size();

        if (pos > 0)
        {
            if (i->load() == 0)
            {
                i->raiseerror(i->StackUnderflowError);
                return;
            }
            (*result)[static_cast<size_t>(pos) - 1] = i->top();
            i->pop();
        }

        if (static_cast<size_t>(pos) < limit)
        {
            Token elem = result->get(pos);
            long idx = pos;
            ++i->EStack().pick(1).data_.long_val;
            Token proc_copy = i->EStack().pick(2);
            i->push(elem);
            i->push(idx);
            i->EStack().push(proc_copy);
        }
        else
        {
            Token res = i->EStack().pick(3);
            i->EStack().pop(5);
            i->push(res);
            i->dec_call_depth();
        }
    }
};

class IMapThreadFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        long pos = i->EStack().pick(1).data_.long_val;
        TokenArray* result = i->EStack().pick(3).data_.array_val;
        TokenArray* sources = i->EStack().pick(4).data_.array_val;
        size_t limit = result->size();

        if (pos > 0)
        {
            if (i->load() == 0)
            {
                i->raiseerror(i->StackUnderflowError);
                return;
            }
            (*result)[static_cast<size_t>(pos) - 1] = i->top();
            i->pop();
        }

        if (static_cast<size_t>(pos) < limit)
        {
            // Push inner_arrays[k][pos] for each k.
            ++i->EStack().pick(1).data_.long_val;
            Token proc_copy = i->EStack().pick(2);
            for (Token const* row = sources->begin(); row != sources->end(); ++row)
                i->push(row->data_.array_val->get(pos));
            i->EStack().push(proc_copy);
        }
        else
        {
            Token res = i->EStack().pick(3);
            i->EStack().pop(6);
            i->push(res);
            i->dec_call_depth();
        }
    }
};

class MapFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(0, sli3::proceduretype);
        i->require_stack_type(1, sli3::arraytype);
        TokenArray* proc = i->top().data_.array_val;
        TokenArray* src = i->pick(1).data_.array_val;
        if (proc->size() == 0 || src->size() == 0)
        {
            // Empty proc or empty source: leave the array untouched.
            // proc is currently on top, array right below — pop proc.
            i->EStack().pop();
            i->pop();
            return;
        }
        TokenArray* result = new TokenArray(*src);
        setup_map_frame_(i, result, i->baselookup(Name("::Map")));
    }
};

class MapIndexedFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(0, sli3::proceduretype);
        i->require_stack_type(1, sli3::arraytype);
        TokenArray* proc = i->top().data_.array_val;
        TokenArray* src = i->pick(1).data_.array_val;
        if (proc->size() == 0 || src->size() == 0)
        {
            i->EStack().pop();
            i->pop();
            return;
        }
        TokenArray* result = new TokenArray(*src);
        setup_map_frame_(i, result, i->baselookup(Name("::MapIndexed")));
    }
};

class MapThreadFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(0, sli3::proceduretype);
        i->require_stack_type(1, sli3::arraytype);
        TokenArray* proc = i->top().data_.array_val;
        TokenArray* outer = i->pick(1).data_.array_val;
        if (outer->empty())
        {
            // No inner arrays -> nothing to do. Leave outer (empty)
            // unchanged on the operand stack.
            i->EStack().pop();
            i->pop();
            return;
        }
        // Validate: every element must be an array of equal length.
        if (!outer->begin()->is_of_type(sli3::arraytype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        size_t n = outer->begin()->data_.array_val->size();
        for (Token const* t = outer->begin(); t != outer->end(); ++t)
        {
            if (!t->is_of_type(sli3::arraytype))
            {
                i->raiseerror(i->ArgumentTypeError);
                return;
            }
            if (t->data_.array_val->size() != n)
            {
                i->raiseerror(i->RangeCheckError);
                return;
            }
        }
        if (proc->size() == 0 || n == 0)
        {
            // Empty proc or zero-length inner: return an empty
            // array (matches NEST 2.x MapThread on this edge case).
            i->EStack().pop();
            i->pop();  // proc
            TokenArray* empty = new TokenArray();
            i->top() = i->new_token<sli3::arraytype>(empty);
            return;
        }
        TokenArray* result = new TokenArray(n, Token());

        // EStack frame for MapThread (additional: source array).
        // Top to bottom:
        //   ::MapThread, pos, proc, result, sources, mark
        i->EStack().pop();  // pop entry function
        i->EStack().push(i->new_token<sli3::marktype>());
        i->EStack().push(i->pick(1));                              // sources (outer)
        i->EStack().push(i->new_token<sli3::arraytype>(result));   // result
        i->EStack().push(i->top());                                // proc
        i->EStack().push(i->new_token<sli3::integertype>(0L));     // pos
        i->EStack().push(i->baselookup(Name("::MapThread")));
        i->inc_call_depth();
        i->pop(2);  // pop sources, proc from ostack
    }
};

//------------------------------------------------------------------------
// Function instances. One per operator, since createcommand() stores a
// pointer and back-references for dispatch and naming.
//------------------------------------------------------------------------

RangeFunction              range_fn;
ArrayFunction              array_fn;
ReverseFunction            reverse_fn;
RotateFunction             rotate_fn;
FlattenFunction            flatten_fn;
SortFunction               sort_fn;
TransposeFunction          transpose_fn;
PartitionFunction          partition_fn;
ArrayloadFunction          arrayload_fn;
ArraystoreFunction         arraystore_fn;
MinMaxFunction<true>       getmax_fn;
MinMaxFunction<false>      getmin_fn;
ValidFunction              valid_fn;
FiniteQDFunction           finiteq_d_fn;
MapFunction                map_fn;
MapIndexedFunction         map_indexed_fn;
MapThreadFunction          map_thread_fn;
IMapFunction               imap_fn;
IMapIndexedFunction        imap_indexed_fn;
IMapThreadFunction         imap_thread_fn;

}  // anonymous namespace

void init_sliarray(SLIInterpreter* i)
{
    i->createcommand("Range",            &range_fn);
    i->createcommand("array_",           &array_fn);
    i->createcommand("Reverse",          &reverse_fn);
    i->createcommand("Rotate",           &rotate_fn);
    i->createcommand("Flatten",          &flatten_fn);
    i->createcommand("Sort",             &sort_fn);
    i->createcommand("Transpose",        &transpose_fn);
    i->createcommand("Partition_a_i_i",  &partition_fn);
    i->createcommand("arrayload",        &arrayload_fn);
    i->createcommand("arraystore",       &arraystore_fn);
    i->createcommand("GetMax",           &getmax_fn);
    i->createcommand("GetMin",           &getmin_fn);
    i->createcommand("valid_a",          &valid_fn);
    i->createcommand("finite_q_d",       &finiteq_d_fn);
    i->createcommand("Map",              &map_fn);
    i->createcommand("MapIndexed_a",     &map_indexed_fn);
    i->createcommand("MapThread_a",      &map_thread_fn);
    i->createcommand("::Map",            &imap_fn);
    i->createcommand("::MapIndexed",     &imap_indexed_fn);
    i->createcommand("::MapThread",      &imap_thread_fn);

    // Axis I bundle step 3f: array_module trailing ops new ABI.

    // Axis I bundle step 3f: trailing-pop array_module ops to new ABI. Map / MapIndexed / MapThread setups stay old (iter frame push).
    array_fn.set_new_abi();
    arraystore_fn.set_new_abi();
    finiteq_d_fn.set_new_abi();
    flatten_fn.set_new_abi();
    partition_fn.set_new_abi();
    range_fn.set_new_abi();
    reverse_fn.set_new_abi();
    rotate_fn.set_new_abi();
    sort_fn.set_new_abi();
    transpose_fn.set_new_abi();
    valid_fn.set_new_abi();
    // Axis I bundle step 4 audit: bodies have no e-stack pop --
    // they relied on the step 3 post-check (which only fires for
    // new-ABI ops). Mark them so step 4's pre-pop applies and the
    // iiterate fast path doesn't push a sentinel that nobody pops.
    arrayload_fn.set_new_abi();
    getmax_fn.set_new_abi();
    getmin_fn.set_new_abi();
}

}  // namespace sli3

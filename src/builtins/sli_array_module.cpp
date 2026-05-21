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
// First / Rest — array | proc | litproc.
//   First returns the leading element; Rest returns a same-type
//   container with the leading element dropped. NEST 2.x defines
//   both in mathematica.sli as `{ 0 get }` / `{ 0 1 erase }`; the
//   C++ versions skip the SLI dispatch overhead and accept all
//   three TokenArray-payload types directly.
//------------------------------------------------------------------------
class FirstFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        unsigned tag = i->top().tag();
        if (tag != sli3::arraytype
            && tag != sli3::proceduretype
            && tag != sli3::litproceduretype)
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        TokenArray const* arr = i->top().data_.array_val;
        if (arr->empty())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        // Copy the element out before overwriting top(): replacing
        // top() may drop the container's last reference and free the
        // storage we're reading from.
        Token first = arr->get(0);
        i->top() = std::move(first);
    }
};

class RestFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        unsigned tag = i->top().tag();
        if (tag != sli3::arraytype
            && tag != sli3::proceduretype
            && tag != sli3::litproceduretype)
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        TokenArray const* src = i->top().data_.array_val;
        if (src->empty())
        {
            i->raiseerror(i->RangeCheckError);
            return;
        }
        TokenArray* result = new TokenArray();  // refs_=1
        result->reserve(src->size() - 1);
        for (Token const* t = src->begin() + 1; t != src->end(); ++t)
            result->push_back(*t);
        // Preserve input container type so {1 2 3} Rest yields a
        // procedure, not an array.
        Token out(i->get_type(tag));
        out.data_.array_val = result;  // consumes the refs_=1
        i->top() = std::move(out);
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
        unsigned int type_id = src->begin()->tag();
        for (Token const* t = src->begin(); t != src->end(); ++t)
        {
            if (!t->type_ || t->tag() != type_id)
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
        unsigned int t_id = arr->begin()->tag();
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
// Map family (C++ port).
//
// Algorithm (per user spec, May 2026):
//   1. Entry op clones the source array into `target` on the estack.
//      The original is LEFT on the ostack so a body-raised error can
//      unwind with the original recoverable.
//   2. Each iter pushes one element (Map) / element+1-based-index
//      (MapIndexed) / column-of-k-elements (MapThread) onto the
//      ostack and runs the body.
//   3. After the body, check ostack.load() == saved_load + 1 — the
//      body must leave exactly one result. If not, raise
//      StackUnderflow.
//   4. Writeback: target[idx-1] = ostack.top(); pop.
//   5. After the final element, overwrite the original on ostack
//      (now at the top, since the body invariant ensures it) with
//      `target` and pop the estack frame.
//
// Frame layout (top → bottom) — common to all three:
//   pick(0)  imap{,indexed,thread}type   marker
//   pick(1)  pos                          proc cursor for body_walk
//   pick(2)  proc                         user procedure
//   pick(3)  idx                          element index (0-based)
//   pick(4)  target / result_copy         array we mutate in place
//   pick(5)  saved_load   (Map/MI)        ostack depth after entry's pop
//   pick(5)  sources      (MapThread)     outer array of inner rows
//   pick(6)  saved_load   (MapThread)
//   pick(5/6) mark        (frame bottom)
//
// All per-iteration work — writeback, depth check, push next args,
// final pop — lives in execute_dispatch_'s body_exhausted switch.
//------------------------------------------------------------------------

namespace {

// Build the iter frame for Map / MapIndexed. Both have the same
// layout; the only difference is the marker tag.
void setup_map_frame(SLIInterpreter* i, sli_typeid marker,
                     TokenArray* target, TokenArray const* proc,
                     long saved_load)
{
    i->EStack().push(i->new_token<sli3::marktype>());
    i->EStack().push(i->new_token<sli3::integertype>(saved_load));
    i->EStack().push(i->new_token<sli3::arraytype>(target));
    i->EStack().push(i->new_token<sli3::integertype>(0));         // idx=0
    i->EStack().push(i->pick(0));                                 // proc
    i->EStack().push(i->new_token<sli3::integertype>(proc->size())); // pos=proc->size()
    i->EStack().push(Token(i->get_type(marker)));
}

}  // namespace

class MapFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(2);
        i->require_stack_type(0, sli3::proceduretype);
        i->require_stack_type(1, sli3::arraytype);
        TokenArray* proc = i->top().data_.array_val;
        TokenArray* src  = i->pick(1).data_.array_val;
        if (proc->size() == 0 || src->size() == 0)
        {
            // Nothing to do — pop the proc, leave the array as result.
            i->pop();
            return;
        }
        // Clone src so a body-raised error leaves the original intact.
        TokenArray* target = new TokenArray(*src);
        // saved_load = ostack depth after we pop the proc.
        long saved_load = static_cast<long>(i->load()) - 1;
        setup_map_frame(i, sli3::imaptype, target, proc, saved_load);
        i->pop();  // pop ONLY the proc; original array stays
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
        TokenArray* src  = i->pick(1).data_.array_val;
        if (proc->size() == 0 || src->size() == 0)
        {
            i->pop();
            return;
        }
        TokenArray* target = new TokenArray(*src);
        long saved_load = static_cast<long>(i->load()) - 1;
        setup_map_frame(i, sli3::imapindexedtype, target, proc, saved_load);
        i->pop();
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
        TokenArray* proc  = i->top().data_.array_val;
        TokenArray* outer = i->pick(1).data_.array_val;
        if (outer->empty())
        {
            // No inner arrays — leave outer (empty) as result.
            i->pop();
            return;
        }
        // Validate: every row is an array, all of equal length.
        if (!outer->begin()->is_of_type(sli3::arraytype))
        {
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
        TokenArray* first = outer->begin()->data_.array_val;
        size_t n = first->size();
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
            i->pop();
            return;
        }
        // Clone the first inner array as the result (NEST 2.20.2
        // convention; sliarray.cc:1801). result_copy gets mutated
        // in place by writebacks.
        TokenArray* result_copy = new TokenArray(*first);
        long saved_load = static_cast<long>(i->load()) - 1;
        i->EStack().push(i->new_token<sli3::marktype>());
        i->EStack().push(i->new_token<sli3::integertype>(saved_load));
        i->EStack().push(i->pick(1));                                 // sources
        i->EStack().push(i->new_token<sli3::arraytype>(result_copy));
        i->EStack().push(i->new_token<sli3::integertype>(0));         // idx=0
        i->EStack().push(i->pick(0));                                 // proc
        i->EStack().push(i->new_token<sli3::integertype>(proc->size())); // pos
        i->EStack().push(Token(i->get_type(sli3::imapthreadtype)));
        i->pop();  // pop ONLY proc
    }
};

//------------------------------------------------------------------------
// Function instances. One per operator, since createcommand() stores a
// pointer and back-references for dispatch and naming.
//------------------------------------------------------------------------

RangeFunction              range_fn;
ArrayFunction              array_fn;
FirstFunction              first_fn;
RestFunction               rest_fn;
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
MapIndexedFunction         mapindexed_fn;
MapThreadFunction          mapthread_fn;

}  // anonymous namespace

void init_sliarray(SLIInterpreter* i)
{
    i->createcommand("Range",            &range_fn);
    i->createcommand("array_",           &array_fn);
    i->createcommand("First_",           &first_fn);
    i->createcommand("Rest_",            &rest_fn);
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
    // Map family (C++ port). Typed-leaf names so mathematica.sli's
    // /Map /MapIndexed /MapThread tries can wire them without
    // colliding with the trie top-level names.
    i->createcommand("Map_a_p",          &map_fn);
    i->createcommand("MapIndexed_a_p",   &mapindexed_fn);
    i->createcommand("MapThread_a_p",    &mapthread_fn);
}

}  // namespace sli3

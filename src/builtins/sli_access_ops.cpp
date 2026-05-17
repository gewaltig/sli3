// PostScript-style access-state operators. The /readonly setter
// narrows the access of an arraytype / proceduretype / litproceduretype
// / dictionarytype / stringtype operand; the rcheck / wcheck / xcheck
// predicates expose the state to SLI.
//
// Each operator dispatches inline on the operand's tag (no trie)
// because the surface is uniform across composite types and we want
// to keep the hot path tight when /readonly is used to seal large
// objects (e.g. systemdict at bootstrap end).

#include "sli_access_ops.h"

#include "sli_access.h"
#include "sli_array.h"
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

// `obj  readonly  ->  obj` — narrow access to ACCESS_READONLY.
// set_access is monotonic, so this is a no-op on objects already at
// readonly or stricter. The Token stays on the stack with the same
// payload pointer; only the payload's flag flips.
class ReadonlyFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        switch (i->top().tag())
        {
        case sli3::arraytype:
        case sli3::proceduretype:
        case sli3::litproceduretype:
            i->top().data_.array_val->set_access(ACCESS_READONLY);
            break;
        case sli3::dictionarytype:
            i->top().data_.dict_val->set_access(ACCESS_READONLY);
            break;
        case sli3::stringtype:
            i->top().data_.string_val->set_access(ACCESS_READONLY);
            break;
        default:
            i->raiseerror(i->ArgumentTypeError);
            return;
        }
    }
};

// `obj  rcheck  ->  bool` — true if read-accessible.
// Composite payloads return their is_readable(). Non-composite
// scalars (integer, double, bool, …) are always readable: they have
// no payload to gate on, and PS treats them as readable by definition.
class RcheckFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        bool readable = true;
        switch (i->top().tag())
        {
        case sli3::arraytype:
        case sli3::proceduretype:
        case sli3::litproceduretype:
            readable = i->top().data_.array_val->is_readable();
            break;
        case sli3::dictionarytype:
            readable = i->top().data_.dict_val->is_readable();
            break;
        case sli3::stringtype:
            readable = i->top().data_.string_val->is_readable();
            break;
        default:
            break;  // scalars are always readable.
        }
        i->pop();
        i->push<bool>(readable);
    }
};

// `obj  wcheck  ->  bool` — true if writable (ACCESS_UNLIMITED).
// Composite payloads dispatch to is_writable(); non-composite
// scalars are reported as writable (PS semantics — assigning a
// new scalar to a variable always works).
class WcheckFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        bool writable = true;
        switch (i->top().tag())
        {
        case sli3::arraytype:
        case sli3::proceduretype:
        case sli3::litproceduretype:
            writable = i->top().data_.array_val->is_writable();
            break;
        case sli3::dictionarytype:
            writable = i->top().data_.dict_val->is_writable();
            break;
        case sli3::stringtype:
            writable = i->top().data_.string_val->is_writable();
            break;
        default:
            break;
        }
        i->pop();
        i->push<bool>(writable);
    }
};

// `obj  xcheck  ->  bool` — true if obj is an executable object.
// In sli3 the executability bit is encoded by typeid: proceduretype
// and litproceduretype are executable; everything else is not.
// (This is orthogonal to the access lattice — xcheck answers "is
// this a procedure?", not "is the access state allowing execute?".)
class XcheckFunction : public SLIFunction
{
public:
    void execute(SLIInterpreter* i) const override
    {
        i->require_stack_load(1);
        unsigned tag = i->top().tag();
        bool exec = (tag == sli3::proceduretype
                     || tag == sli3::litproceduretype);
        i->pop();
        i->push<bool>(exec);
    }
};

ReadonlyFunction readonly_fn;
RcheckFunction   rcheck_fn;
WcheckFunction   wcheck_fn;
XcheckFunction   xcheck_fn;

}  // namespace

void init_access_ops(SLIInterpreter* i)
{
    i->createcommand("readonly", &readonly_fn);
    i->createcommand("rcheck",   &rcheck_fn);
    i->createcommand("wcheck",   &wcheck_fn);
    i->createcommand("xcheck",   &xcheck_fn);

    readonly_fn.set_new_abi();
    rcheck_fn.set_new_abi();
    wcheck_fn.set_new_abi();
    xcheck_fn.set_new_abi();
}

}  // namespace sli3

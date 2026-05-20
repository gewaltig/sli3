#ifndef SLI_COW_H
#define SLI_COW_H

// Clone-on-write helpers for in-place container mutators.
//
// Used by extension operators (append, prepend) that grow a shared
// container in place. Without privatising the slot first, mutating a
// container that's also referenced from a dict entry or another stack
// slot leaks the mutation to every holder — the classic case is the
// `{ << >> begin }` parser-shared litproc inside SLIFunctionWrapper.
//
// NOT included by sli_op_bodies.h: that header is pulled into the
// dispatcher TU, and adding helpers there perturbs the inline layout
// of the hot ops even when they're not called.

#include "sli_array.h"
#include "sli_interpreter.h"
#include "sli_string.h"
#include "sli_token.h"

#include <cstddef>

namespace sli3
{

static inline TokenArray* cow_array_slot(SLIInterpreter* i, size_t slot_depth)
{
    TokenArray* arr = i->pick(slot_depth).data_.array_val;
    if (arr->references() > 1)
    {
        TokenArray* fresh = new TokenArray(*arr);
        Token& slot = i->pick(slot_depth);
        arr->remove_reference();
        slot.data_.array_val = fresh;
        arr = fresh;
    }
    return arr;
}

// SLIString's copy ctor is deleted, so the clone is constructed from
// the underlying std::string.
static inline SLIString* cow_string_slot(SLIInterpreter* i, size_t slot_depth)
{
    SLIString* sv = i->pick(slot_depth).data_.string_val;
    if (sv->references() > 1)
    {
        SLIString* fresh = new SLIString(sv->str());
        Token& slot = i->pick(slot_depth);
        sv->remove_reference();
        slot.data_.string_val = fresh;
        sv = fresh;
    }
    return sv;
}

}  // namespace sli3

#endif

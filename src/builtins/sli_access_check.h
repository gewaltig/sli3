#ifndef SLI_ACCESS_CHECK_H
#define SLI_ACCESS_CHECK_H

// Inline helpers for PostScript-style access checks on composite
// payloads (TokenArray, Dictionary, SLIString). Each returns true on
// success and raises the appropriate error + returns false on failure,
// so callers can write:
//
//     if (!require_readable(arr, i)) return;
//     ...
//
// The check itself is one byte-load + compare; the raise path is cold
// (gated with __builtin_expect). Templated on Payload so the same body
// works for all three composite types without virtual dispatch.

#include "sli_interpreter.h"

namespace sli3
{

template <typename Payload>
[[nodiscard]] static inline bool
require_readable(Payload* p, SLIInterpreter* i)
{
    if (__builtin_expect(!p->is_readable(), 0))
    {
        i->raiseerror(i->InvalidAccessError);
        return false;
    }
    return true;
}

template <typename Payload>
[[nodiscard]] static inline bool
require_writable(Payload* p, SLIInterpreter* i)
{
    if (__builtin_expect(!p->is_writable(), 0))
    {
        i->raiseerror(i->WriteProtectedError);
        return false;
    }
    return true;
}

}  // namespace sli3

#endif

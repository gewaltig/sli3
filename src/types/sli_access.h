#ifndef SLI_ACCESS_H
#define SLI_ACCESS_H

#include <cstdint>

namespace sli3
{

/**
 * PostScript-style access state for composite objects (TokenArray,
 * Dictionary, SLIString). Stored on the payload — not the Token —
 * so every Token referencing a given payload observes the same
 * state. Default is ACCESS_UNLIMITED; the field is a uint8_t,
 * zero-initialised by container constructors.
 *
 * The lattice is monotonically narrowing: once a payload is marked
 * readonly (or stricter) it stays that way for the lifetime of the
 * object. Cloning a payload (TokenArray copy ctor, clonedict, …)
 * produces a fresh object back at ACCESS_UNLIMITED.
 *
 *   unlimited → readonly → executeonly → noaccess
 *
 * Wave 1 wires the field, the predicates, the readonly / rcheck /
 * wcheck / xcheck operators, and gates every composite mutation
 * site on is_writable(). Wave 2 adds the executeonly + noaccess
 * setters and read-path checks on the headline operators (get, keys,
 * values, cva, print/pprint). Wave 3 extends the read gates to the
 * functional read-and-copy operators (join, search, insert, replace,
 * erase, getinterval, insertelement, clonedict, known, cvi_s, cvd_s,
 * eq on strings) and path-put descent.
 *
 * Errors: denied mutations raise `WriteProtected`; denied reads
 * raise `InvalidAccess`. Both correspond to PostScript's single
 * `invalidaccess` condition; sli3 keeps them split so script
 * authors can branch on errordict /errorname.
 */
enum AccessState : uint8_t
{
    ACCESS_UNLIMITED   = 0,  // read + write + execute (zero-init default)
    ACCESS_READONLY    = 1,  // read + execute, no write
    ACCESS_EXECUTEONLY = 2,  // execute, no read or write           (Wave 2)
    ACCESS_NOACCESS    = 3,  // no access — every op raises          (Wave 2)
};

}  // namespace sli3

#endif

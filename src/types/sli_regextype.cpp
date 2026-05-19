#include "sli_regextype.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"

#include <iostream>

namespace sli3
{

void RegexType::serialize(Token const& t, Writer& w) const
{
    Regex* r = t.data_.regex_val;
    auto [id, is_new] = w.intern_object(r);
    w.write_u32(id);
    if (is_new)
    {
        // POSIX regex_t is opaque -- the only portable way to round-trip
        // is to save the source pattern + flags and recompile on load.
        w.write_string(r ? r->pattern() : std::string());
        w.write_u32(static_cast<uint32_t>(r ? r->flags() : 0));
    }
}

void RegexType::deserialize(Reader& r, Token& t) const
{
    uint32_t id = r.read_u32();
    Regex* rx = static_cast<Regex*>(r.lookup_object(id));
    if (rx)
    {
        rx->add_reference();
    }
    else
    {
        std::string pat = r.read_string();
        int flags = static_cast<int>(r.read_u32());
        rx = new Regex;
        int e = rx->compile(pat, flags);
        if (e != 0)
        {
            // Re-compilation failed (e.g. saved with a different
            // POSIX flavor). Best-effort: keep the wrapper alive with
            // compiled_=false so SLI code can still inspect it; warn
            // on stderr so the user knows the snapshot lost fidelity.
            std::cerr << "RegexType::deserialize: regcomp failed on "
                      << "restored pattern \"" << pat << "\" (code " << e
                      << "); regex left uncompiled.\n";
        }
        r.register_object(id, rx);
    }
    t.type_ = const_cast<RegexType*>(this);
    t.data_.regex_val = rx;
}

}  // namespace sli3

#include "sli_regextype.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"

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
        // On compile failure (e.g. saved with a different POSIX flavor)
        // the wrapper stays alive with compiled_=false. SLI code can
        // introspect that state via typeinfo and :regerror. We don't
        // surface a stderr warning from deserialize because read_token
        // is otherwise side-effect-free.
        (void) rx->compile(pat, flags);
        r.register_object(id, rx);
    }
    t.type_ = const_cast<RegexType*>(this);
    t.data_.regex_val = rx;
}

}  // namespace sli3

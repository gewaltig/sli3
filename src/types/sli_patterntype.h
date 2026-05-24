#ifndef SLI_PATTERNTYPE_H
#define SLI_PATTERNTYPE_H

#include "sli_graphics.h"
#include "sli_token.h"
#include "sli_type.h"

namespace sli3
{

/**
 * Token type for a Cairo source pattern (cairo_pattern_t), wrapped by
 * CairoPattern. Same opaque-refcounted-non-serializable shape as
 * GraphicsContextType -- patterns hold internal state (color stops,
 * matrix) that doesn't survive a snapshot, and reconstructing them
 * across a serialize/deserialize round-trip is not a use case we
 * support.
 */
class PatternType : public SLIType
{
public:
    PatternType(SLIInterpreter* sli, char const name[], sli_typeid type)
        : SLIType(sli, name, type)
    {
        executable_ = false;
        needs_refcount_ = true;
    }

    refcount_t add_reference(Token const& t) const override
    {
        if (t.data_.pattern_val) return t.data_.pattern_val->add_reference();
        return 0;
    }

    void remove_reference(Token& t) const override
    {
        if (t.data_.pattern_val
            and t.data_.pattern_val->remove_reference() == 0)
        {
            t.type_ = 0;
            t.data_ = Token::value();
        }
    }

    void clear(Token& t) const override
    {
        remove_reference(t);
        t.data_.pattern_val = 0;
    }

    refcount_t references(Token const& t) const override
    {
        if (t.data_.pattern_val) return t.data_.pattern_val->references();
        return 0;
    }

    bool compare(Token const& t1, Token const& t2) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
};

}  // namespace sli3

#endif

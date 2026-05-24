#ifndef SLI_GRAPHICSCONTEXTTYPE_H
#define SLI_GRAPHICSCONTEXTTYPE_H

#include "sli_graphics.h"
#include "sli_token.h"
#include "sli_type.h"

namespace sli3
{

/**
 * Token type for the Cairo + SDL2 drawing surface owned by a
 * GraphicsContext. Modeled on IstreamType / OstreamType: opaque
 * payload, refcounted, non-executable, non-serializable.
 *
 * Constructed only by the /newpage operator; SLI code cannot
 * synthesize one with the parser.
 */
class GraphicsContextType : public SLIType
{
public:
    GraphicsContextType(SLIInterpreter* sli, char const name[], sli_typeid type)
        : SLIType(sli, name, type)
    {
        executable_ = false;
        needs_refcount_ = true;
    }

    refcount_t add_reference(Token const& t) const override
    {
        if (t.data_.graphics_val) return t.data_.graphics_val->add_reference();
        return 0;
    }

    void remove_reference(Token& t) const override
    {
        if (t.data_.graphics_val
            and t.data_.graphics_val->remove_reference() == 0)
        {
            t.type_ = 0;
            t.data_ = Token::value();
        }
    }

    void clear(Token& t) const override
    {
        remove_reference(t);
        t.data_.graphics_val = 0;
    }

    refcount_t references(Token const& t) const override
    {
        if (t.data_.graphics_val) return t.data_.graphics_val->references();
        return 0;
    }

    bool compare(Token const& t1, Token const& t2) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
};

}  // namespace sli3

#endif

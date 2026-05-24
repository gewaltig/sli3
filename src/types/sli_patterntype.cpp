#include "sli_patterntype.h"

#include "sli_serialize.h"

#include <iostream>
#include <string>

namespace sli3
{

bool PatternType::compare(Token const& t1, Token const& t2) const
{
    if (t1.type_ != t2.type_) return false;
    return t1.data_.pattern_val == t2.data_.pattern_val;
}

// Patterns hold cairo_pattern_t state (color stops, matrix, surface
// references) that isn't snapshot-safe. Same "warn + write nothing /
// deserialize as null" contract as graphicscontexttype and the
// streams. Any subsequent /setpattern on the null token raises.
void PatternType::serialize(Token const&, Writer&) const
{
    std::cerr << "sli3: pattern is not serializable; "
                 "replaced with null pattern\n";
}

void PatternType::deserialize(Reader&, Token& t) const
{
    t.type_ = const_cast<PatternType*>(this);
    t.data_.pattern_val = nullptr;
}

}  // namespace sli3

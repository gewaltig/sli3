#include "sli_iostream.h"

#include <istream>
#include <ostream>

namespace sli3
{

SLIistream::~SLIistream()
{
    if (owns_) delete stream_;
}

SLIostream::~SLIostream()
{
    if (owns_) delete stream_;
}

}  // namespace sli3

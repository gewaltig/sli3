#ifndef SLI_IOSTREAM_H
#define SLI_IOSTREAM_H
#include "sli_lockptr.h"
#include <iostream>
namespace sli3
{

  class SLIistream: public lockPTR<std::istream>
  {
  };

  class SLIostream: public lockPTR<std::ostream>
  {
  };

}

#endif SLI_IOSTREAM_H

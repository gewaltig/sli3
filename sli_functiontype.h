#ifndef SLI_FUNCTIONTYPE_H
#define SLI_FUNCTIONTYPE_H
#include "sli_type.h"

namespace sli3
{
  class FunctionType: public SLIType
  {
  public:
  FunctionType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :SLIType(sli, name, type){}
    
    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
    std::ostream & pprint(std::ostream&, const Token &) const;

    void execute(Token &);
  };


}
#endif

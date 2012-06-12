#ifndef SLI_INTEGERTYPE_H
#define SLI_INTEGERTYPE_H

#include "sli_type.h"
#include "SLI_token.h"
#include "sli_array.h"
namespace sli3
{
  class IntegerType: public SLIType
  {
  public:
    IntegerType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
  };

  class DoubleType: public SLIType
  {
  public:
    DoubleType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;

  };

  class BoolType: public SLIType
  {
  public:
    BoolType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;

  };
   
}

#endif

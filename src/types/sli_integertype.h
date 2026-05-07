#ifndef SLI_INTEGERTYPE_H
#define SLI_INTEGERTYPE_H

#include "sli_type.h"
#include "sli_token.h"
#include "sli_array.h"
namespace sli3
{
  class IntegerType: public SLIType
  {
  public:
    IntegerType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class DoubleType: public SLIType
  {
  public:
    DoubleType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class BoolType: public SLIType
  {
  public:
    BoolType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };
   
}

#endif

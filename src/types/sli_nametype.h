#ifndef SLI_NAMETYPE_H
#define SLI_NAMETYPE_H
#include "sli_type.h"
namespace sli3
{

  class LiteralType: public SLIType
  {
  public:
  LiteralType(SLIInterpreter *sli, char const name[], sli_typeid type, bool exec=false)
	:SLIType(sli, name, type,exec){}

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class NameType: public LiteralType
  {
  public:
      NameType(SLIInterpreter *sli, char const name[], sli_typeid type)
	:LiteralType(sli, name, type,true)
      {}

    void execute(Token &) override;

    std::ostream & print(std::ostream&, const Token &) const override;
  };

  class SymbolType: public LiteralType
  {
  public:
  SymbolType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}

    std::ostream & print(std::ostream&, const Token &) const override;
  };

  class MarkType: public LiteralType
  {
  public:
  MarkType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}
    std::ostream & print(std::ostream&, const Token &) const override;
  };


  /**
   * Operators are special types that are executed inside the interpreter loop.
   */

  template<sli_typeid type>
  class OperatorType: public LiteralType
  {
  public:
  OperatorType(SLIInterpreter *sli, char const name[])
    :LiteralType(sli, name, type, true){}

    std::ostream & print(std::ostream& out, const Token &) const override
      {
	out << "::" << get_typename();
	return out;
      }
  };





}

#endif

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

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
  };

  class NameType: public LiteralType
  {
  public:
      NameType(SLIInterpreter *sli, char const name[], sli_typeid type)
	:LiteralType(sli, name, type,true)
      {}
    
    void execute(Token &);

    std::ostream & print(std::ostream&, const Token &) const;
  };

  class SymbolType: public LiteralType
  {
  public:
  SymbolType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}

    std::ostream & print(std::ostream&, const Token &) const;
  };

  class MarkType: public LiteralType
  {
  public:
  MarkType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}
    std::ostream & print(std::ostream&, const Token &) const;
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

    std::ostream & print(std::ostream& out, const Token &) const
      {
	out << "::" << get_typename();
	return out;
      }
  };





}

#endif

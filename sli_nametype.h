#ifndef SLI_NAMETYPE_H
#define SLI_NAMETYPE_H
#include "sli_type.h"
namespace sli3
{

  class LiteralType: public SLIType
  {
  public:
      LiteralType(SLIInterpreter *sli, char const name[], sli_typeid type)
	:SLIType(sli, name, type){executable_=false;}

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
  };

  class SymbolType: public LiteralType
  {
  public:
  SymbolType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}

    std::ostream & print(std::ostream&, const Token &) const;
  };

  /**
   * Marks is a special literal. To process it more efficiently,
   * we devote a special type to it.
   */
  class MarkType: public LiteralType
  {
  public:
  MarkType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LiteralType(sli, name, type){}
    
    std::ostream & print(std::ostream&, const Token &) const;
  };

  class NameType: public LiteralType
  {
  public:
      NameType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :LiteralType(sli, name, type)
      {executable_ =true;}
    
    void execute(Token &);

    std::ostream & print(std::ostream&, const Token &) const;
  };





}

#endif

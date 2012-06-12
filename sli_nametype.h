#ifndef SLI_NAMETYPE_H
#define SLI_NAMETYPE_H
#include "sli_type.h"
namespace sli3
{

  class NameType: public SLIType
  {
  public:
      NameType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :SLIType(sli, name, type){}

      bool compare(const Token&t1, const Token&t2) const;
      std::ostream & print(std::ostream&, const Token &) const;
  };

  class LiteralType: public NameType
  {
  public:
      LiteralType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :NameType(sli, name, type){}

      std::ostream & print(std::ostream&, const Token &) const;
  };

  class SymbolType: public NameType
  {
  public:
      SymbolType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :NameType(sli, name, type){}

      std::ostream & print(std::ostream&, const Token &) const;
  };


}

#endif

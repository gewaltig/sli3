#ifndef SLI_ARRAYTYPE_H
#define SLI_ARRAYTYPE_H

#include "sli_type.h"
#include "sli_token.h"
#include "sli_array.h"
#include <iostream>
#include <cassert>
namespace sli3
{
  
  class ArrayType: public SLIType
  {
  public:
    ArrayType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    refcount_t add_reference(Token const& t) const
    {
	return t.data_.array_val->add_reference();
    }

    void remove_reference(Token &t) const
    {
      if(t.data_.array_val->remove_reference() ==0)
	{
	  t.type_=0;
	  t.data_=Token::value();
	}
    }

    void clear(Token &t) const
    {
      remove_reference(t);
      t.data_.array_val=0;
    }

    refcount_t references(Token const &t) const
    {
      if(t.data_.array_val!=0)
	return t.data_.array_val->remove_reference();
      else
	return 0;
    }

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
  };

  class LitprocedureType: public ArrayType
  {
  public:
  LitprocedureType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :ArrayType(sli, name, type){}
    std::ostream & print(std::ostream&, const Token &) const;

    void execute(Token &);
  };

  class ProcedureType: public LitprocedureType
  {
  public:
  ProcedureType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LitprocedureType(sli, name, type){}
    
    void execute(Token &);

  };
}
#endif

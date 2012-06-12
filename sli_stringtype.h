#ifndef SLI_STRINGYPE_H
#define SLI_STRINGTYPE_H

#include "sli_type.h"
#include "sli_token.h"
#include "sli_string.h"
#include <iostream>
#include <cassert>
namespace sli3
{
  
  class StringType: public SLIType
  {
  public:
    StringType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type){}

    refcount_t add_reference(Token const& t) const
    {
      if(t.data_.string_val !=0)
	return t.data_.string_val->add_reference();
      else
	return 0;
    }

    void remove_reference(Token &t) const
    {
      if(t.data_.string_val!=0)
	{
	  if(t.data_.string_val->remove_reference() ==0)
	    {
	      t.type_=0;
	      t.data_=Token::value();
	    }
	}
    }

    void clear(Token &t) const
    {
      remove_reference(t);
      t.data_.string_val=0;
    }

    refcount_t references(Token const &t) const
    {
      if(t.data_.string_val!=0)
	return t.data_.string_val->remove_reference();
      else
	return 0;
    }

    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
  };
}
#endif

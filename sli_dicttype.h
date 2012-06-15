#ifndef SLI_DICTTYPE_H
#define SLI_DICTTYPE_H
#include "sli_type.h"
#include "sli_dictionary.h"
#include <cassert>
namespace sli3
{

  class DictionaryType: public SLIType
  {
  public:
      DictionaryType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :SLIType(sli, name, type){}

    refcount_t add_reference(Token const& t) const
    {
      if(t.data_.dict_val !=0)
	return t.data_.dict_val->add_reference();
      else
	return 0;
    }

    void remove_reference(Token &t) const
    {
      if(t.data_.dict_val!=0)
	t.data_.dict_val->remove_reference();
    }

    void clear(Token &t) const
    {
      remove_reference(t);
      t.data_.dict_val=0;
      t.type_=0;
    }


    refcount_t references(Token const &t) const
    {
      if(t.data_.dict_val !=0)
	return t.data_.dict_val->references();
      else 
	return 0;
    }

      bool compare(const Token&t1, const Token&t2) const;
      std::ostream & print(std::ostream&, const Token &) const;
  };
}

#endif

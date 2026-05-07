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

    refcount_t add_reference(Token const& t) const override
    {
      // Null-payload: silent no-op. See SLIType::clear convention.
      if(t.data_.dict_val !=0)
	return t.data_.dict_val->add_reference();
      else
	return 0;
    }

    void remove_reference(Token &t) const override
    {
      if (t.data_.dict_val == 0) return;
      if(t.data_.dict_val->remove_reference() ==0)
	{
	  t.type_=0;
	  t.data_.dict_val=0;
	}
    }

    void clear(Token &t) const override
    {
      remove_reference(t);
      t.data_.dict_val=0;
      t.type_=0;
    }


    refcount_t references(Token const &t) const override
    {
      if (t.data_.dict_val == 0) return 0;
      return t.data_.dict_val->references();
    }

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    std::ostream & pprint(std::ostream&, const Token &) const override;
  };
}

#endif

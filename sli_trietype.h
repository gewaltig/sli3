#ifndef SLI_TRIETYPE_H
#define SLI_TRIETYPE_H
#include "sli_type.h"
#include "sli_trie.h"
#include <cassert>
namespace sli3
{

  class TrieType: public SLIType
  {
  public:
      TrieType(SLIInterpreter *sli, char const name[], sli_typeid type)
	  :SLIType(sli, name, type){}

    refcount_t add_reference(Token const& t) const
    {
      return t.data_.trie_val->add_reference();
    }

    void remove_reference(Token &t) const
    {
      if(t.data_.trie_val->remove_reference() ==0)
	{
	  t.type_=0;
	  t.data_.trie_val=0;
	}
    }

    void clear(Token &t) const
    {
      remove_reference(t);
      t.data_.trie_val=0;
      t.type_=0;
    }


    refcount_t references(Token const &t) const
    {
      return t.data_.trie_val->references();
    }


    bool compare(const Token&t1, const Token&t2) const;
    std::ostream & print(std::ostream&, const Token &) const;
    std::ostream & pprint(std::ostream&, const Token &) const;
  };
}

#endif
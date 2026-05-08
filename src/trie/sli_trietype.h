#ifndef SLI_TRIETYPE_H
#define SLI_TRIETYPE_H
#include "sli_type.h"
#include "sli_type_trie.h"
#include <cassert>
namespace sli3
{

  class TrieType: public SLIType
  {
  public:
      TrieType(SLIInterpreter *sli, char const name[], sli_typeid type)
        :SLIType(sli, name, type,true){ needs_refcount_ = true; }

    refcount_t add_reference(Token const& t) const override
    {
      // Null-payload convention: silent no-op. See SLIType::clear.
      if (t.data_.trie_val == 0) return 0;
      return t.data_.trie_val->add_reference();
    }

    void remove_reference(Token &t) const override
    {
      if (t.data_.trie_val == 0) return;
      if(t.data_.trie_val->remove_reference() ==0)
	{
	  t.type_=0;
	  t.data_.trie_val=0;
	}
    }

    void clear(Token &t) const override
    {
      remove_reference(t);
      t.data_.trie_val=0;
      t.type_=0;
    }


    refcount_t references(Token const &t) const override
    {
      if (t.data_.trie_val == 0) return 0;
      return t.data_.trie_val->references();
    }


    bool compare(const Token&t1, const Token&t2) const override
    {
      if (t1.data_.trie_val == 0 || t2.data_.trie_val == 0)
        return t1.data_.trie_val == t2.data_.trie_val;
      return *(t1.data_.trie_val) == *(t2.data_.trie_val);
    }

    // Trie dispatch: pick the matching variant based on the operand
    // stack and replace this trie's slot on the execution stack with
    // the matched function (or push it to the operand stack if it is
    // not executable). The execute_dispatch_ loop has the same logic
    // inlined as a switch case; this override mirrors it so the
    // simpler execute_ loop (used during startup) handles tries too.
    void execute(Token &t) override
    {
      const Token &found = t.data_.trie_val->lookup(sli_->OStack());
      if (found.is_executable())
      {
        sli_->EStack().top() = found;
      }
      else
      {
        sli_->push(found);
        sli_->EStack().pop();
      }
    }

    std::ostream & print(std::ostream& out, const Token &t) const override
      {
        if (t.data_.trie_val == 0) return out << "+null+";
        out << '+' << t.data_.trie_val->get_name() << '+';
        return out;
      }

    std::ostream & pprint(std::ostream&out, const Token &t) const override
      {
        if (t.data_.trie_val == 0) return out << "+null+";
        out << '+' << t.data_.trie_val->get_name() << '+';
        return out;
      }

  };
}

#endif

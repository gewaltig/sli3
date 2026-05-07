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

    refcount_t add_reference(Token const& t) const override
    {
      // Null-payload convention: silent no-op. A null payload is
      // reachable via base SLIType::deserialize on a missing
      // serialize override and via Token::value() zero-fill on a
      // freshly cleared slot. See SLIType::clear (sli_type.h:97).
      if (t.data_.array_val == 0) return 0;
      return t.data_.array_val->add_reference();
    }

    void remove_reference(Token &t) const override
    {
      if (t.data_.array_val == 0) return;
      if(t.data_.array_val->remove_reference() ==0)
	{
	  t.type_=0;
	  t.data_=Token::value();
	}
    }

    void clear(Token &t) const override
    {
      remove_reference(t);
      t.data_.array_val=0;
    }

    refcount_t references(Token const &t) const override
    {
      if(t.data_.array_val!=0)
	return t.data_.array_val->references();
      return 0;
    }

    bool compare(const Token&t1, const Token&t2) const override;
    std::ostream & print(std::ostream&, const Token &) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class LitprocedureType: public ArrayType
  {
  public:
  LitprocedureType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :ArrayType(sli, name, type){}
    std::ostream & print(std::ostream&, const Token &) const override;

    void execute(Token &) override;
  };

  class ProcedureType: public LitprocedureType
  {
  public:
  ProcedureType(SLIInterpreter *sli, char const name[], sli_typeid type)
    :LitprocedureType(sli, name, type){}

    std::ostream & print(std::ostream&, const Token &) const override;
    void execute(Token &) override;

  };
}
#endif

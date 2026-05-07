#include "sli_arraytype.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"

namespace sli3
{
    bool ArrayType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.array_val == t2.data_.array_val;
    }

    void ArrayType::serialize(Token const& t, Writer& w) const
    {
	TokenArray* arr = t.data_.array_val;
	auto [id, is_new] = w.intern_object(arr);
	w.write_u32(id);
	if (is_new) arr->serialize_body(w);
    }

    void ArrayType::deserialize(Reader& r, Token& t) const
    {
	uint32_t id = r.read_u32();
	TokenArray* arr = static_cast<TokenArray*>(r.lookup_object(id));
	if (arr)
	{
	    arr->add_reference();
	}
	else
	{
	    arr = new TokenArray();
	    r.register_object(id, arr);
	    arr->deserialize_body(r, *sli_);
	}
	t.type_ = const_cast<ArrayType*>(this);
	t.data_.array_val = arr;
    }

    std::ostream& ArrayType::print(std::ostream& out, const Token &t) const
    {
	if (t.data_.array_val !=0)
	{
	    return out <<'[' << *t.data_.array_val <<']';
	}
	else
	    return out << "[\0]";
	
    }

    std::ostream& LitprocedureType::print(std::ostream& out, const Token &t) const
    {
	if (t.data_.array_val !=0)
	{
	  return out << "{{" << *t.data_.array_val << "}}";
	}
	else
	    return out << "{{\0}}";
	
    }

  void LitprocedureType::execute(Token &t)
    {
      t.type_=sli_->get_type(sli3::proceduretype);
      sli_->push(t);
      sli_->EStack().pop();
    }
 
  void ProcedureType::execute(Token &)
  {
    sli_->EStack().push(sli_->new_token<sli3::integertype>(0));
    sli_->EStack().push(sli_->baselookup(sli_->iiterate_name));
    sli_->inc_call_depth();
    sli_->EStack().dump(std::cerr);
  }
}

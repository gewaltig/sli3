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
	// Literal procedures print with double braces so they can be
	// visually distinguished from executable procedures in
	// stack/dump output. PostScript convention; matches NEST 2.x.
	if (t.data_.array_val !=0)
	{
	  return out << "{{" << *t.data_.array_val << "}}";
	}
	else
	    return out << "{{}}";

    }

    std::ostream& ProcedureType::print(std::ostream& out, const Token &t) const
    {
	// Executable procedures print with single braces — PostScript
	// convention. Until Stage 1.4 this fell through to
	// LitprocedureType::print and produced "{{...}}" which is
	// indistinguishable from a literal procedure.
	if (t.data_.array_val !=0)
	{
	  return out << "{" << *t.data_.array_val << "}";
	}
	else
	    return out << "{}";
    }

  void LitprocedureType::execute(Token &t)
    {
      // `t` is the Token currently on top of the e-stack. The
      // previous implementation mutated t.type_ in place before
      // copying to the operand stack, which is correct only because
      // the e-stack slot is destroyed immediately afterwards. If
      // sli_->push(t) ever threw, the e-stack would be left holding
      // a procedure-typed Token in the litprocedure slot — a
      // self-recursing failure.
      //
      // Move the payload out of the e-stack slot first, retag the
      // copy as proceduretype, push to the operand stack, then pop
      // the now-empty e-stack slot. If push throws, the e-stack
      // slot is moved-from (type_==0); the dispatcher's next
      // iteration will see invalid-Token and unwind cleanly.
      Token copy(std::move(t));
      copy.type_ = sli_->get_type(sli3::proceduretype);
      sli_->push(copy);
      sli_->EStack().pop();
    }

  void ProcedureType::execute(Token &)
  {
    sli_->EStack().push(sli_->new_token<sli3::integertype>(0));
    sli_->EStack().push(sli_->baselookup(sli_->iiterate_name));
    sli_->inc_call_depth();
    // EStack().dump(std::cerr) was here as leftover debug; removed
    // in Stage 1.4 — the dispatcher invokes this on every procedure
    // call and the dump was flooding stderr.
  }
}

#include "sli_arraytype.h"
#include "sli_interpreter.h"

namespace sli3
{
    bool ArrayType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.array_val == t2.data_.array_val;
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
      
    }
 
  void ProcedureType::execute(Token &t)
  {
      std::cerr << "executing\n";
  }
}

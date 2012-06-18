#include "sli_functiontype.h"
#include "sli_interpreter.h"
#include "sli_function.h"

namespace sli3
{

  bool FunctionType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.func_val == t2.data_.func_val;
  }


  std::ostream & FunctionType::pprint(std::ostream& out, const Token & t) const
  {
    if(t.data_.func_val)
      return out << '-'<<t.data_.func_val->get_name()<< '-';
    else
	return out;
  }

  std::ostream & FunctionType::print(std::ostream& out, const Token & t) const
  {
    if(t.data_.func_val)
	return out <<t.data_.func_val->get_name() ;
    else
	return out;
  }
   
  void FunctionType::execute(Token &t)
  {
    if(t.data_.func_val)
      t.data_.func_val->execute(sli_);
  }

}

#include "sli_functiontype.h"
#include "sli_interpreter.h"
#include "sli_function.h"
#include "sli_exceptions.h"

namespace sli3
{

  bool IstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.istream_val == t2.data_.istream_val;
  }

  bool XIstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.istream_val == t2.data_.istream_val;
  }

  bool OstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.ostream_val == t2.data_.ostream_val;
  }
   
  void XIstreamType::execute(Token &t)
  {
    if(t.data_.istream_val)
    {
	sli_->EStack().push(t);
	sli_->EStack().push(sli_->iparse_name);
    }
    else
	throw IOError();
  }

}

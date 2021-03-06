#include "sli_type.h"
#include "sli_token.h"
#include "sli_interpreter.h"
namespace sli3
{
    SLIType::SLIType(SLIInterpreter *sli, char const name[], sli_typeid id, bool exec)
	: sli_(sli),
	  name_(name),
	  id_(id),
	  executable_(exec)
    {}

  void SLIType::clear(Token& t) const
    {
      t.type_=0;
      t.data_ = Token::value(); //This clears all fields to 0
    }
 
    void SLIType::raise_type_mismatch_(unsigned int id) const
    {
	assert(sli_);
	SLIType *required=sli_->get_type(id);
	assert(required);
	throw TypeMismatch(required->name_.toString(), name_.toString());
    }

    void SLIType::execute(Token &t)
    {
	sli_->push(t);
	sli_->EStack().pop();
    }

    std::ostream & SLIType::print(std::ostream& out, const Token &) const
    {
	return out << '-' << name_ << '-';
    }

    std::ostream & SLIType::pprint(std::ostream& out, const Token & t) const
    {
	return out << '-' << name_ << " (" << t.data_.func_val << ") -";
    }

 
}

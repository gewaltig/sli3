#include "sli_stringtype.h"
#include "sli_interpreter.h"

namespace sli3
{
    bool StringType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	  return false;
	return *t1.data_.string_val == *t2.data_.string_val;
    }

    std::ostream& StringType::print(std::ostream& out, const Token &t) const
    {
	if (t.data_.string_val !=0)
	{
	    return out << *t.data_.string_val;
	}
	else
	    return out;
	
    }
}

#include "sli_type.h"
#include "sli_token.h"

namespace sli3
{
    SLIType::SLIType(SLIInterpreter *sli, char const name[], sli_typeid id)
	: sli_(sli),
	  name_(name),
	  id_(id)
    {}

  void SLIType::clear(Token& t) const
    {
      t.type_=0;
      t.data_ = Token::value(); //This clears all fields to 0
    }
 
    
}

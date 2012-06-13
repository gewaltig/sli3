#ifndef SLI_TOKENUTULS_H
#define SLI_TOKENUTILS_H
#include "sli_token.h"
#include "sli_type.h"
#include "sli_array.h"
#include "sli_exceptions.h"
namespace sli3
{
  template<sli_typeid, class D>
   D& token_value(Token &t)
  {
    return (D) t;
  }

  template<>
    TokenArray *& token_value<sli3::proceduretype,TokenArray *>(Token &t)
    {
      if(t.type_ == 0 or ! t.type_->is_type(sli3::proceduretype))
	throw TypeMismatch();
      else
	return t.data_.array_val;
    }

  template<>
    TokenArray *& token_value<sli3::arraytype,TokenArray *>(Token &t)
    {
      if(t.type_ == 0 or ! t.type_->is_type(sli3::arraytype))
	throw TypeMismatch();
      else
	return t.data_.array_val;
    }
}
#endif

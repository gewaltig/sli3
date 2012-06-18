#include "sli_integertype.h"
#include <iostream>

namespace sli3
{

    bool IntegerType::compare(const Token&t1, const Token&t2) const
    {
      if(t1.type_ != t2.type_)
	return false;
      return t1.data_.long_val == t2.data_.long_val;
    }

    std::ostream & IntegerType::print(std::ostream&out, const Token &t) const
    {
	return out << t.data_.long_val;
    }

    bool DoubleType::compare(const Token&t1, const Token&t2) const
    {
      if(t1.type_ != t2.type_)
	return false;
      return t1.data_.double_val == t2.data_.double_val;
    }

    std::ostream & DoubleType::print(std::ostream&out, const Token &t) const
    {
	return out << t.data_.double_val;
    }

    bool BoolType::compare(const Token&t1, const Token&t2) const
    {
      if(t1.type_ != t2.type_)
	return false;
      return t1.data_.bool_val == t2.data_.bool_val;
    }

    std::ostream & BoolType::print(std::ostream&out, const Token &t) const
    {
	return (out <<( t.data_.bool_val ? "true":"false" ));
    }


}

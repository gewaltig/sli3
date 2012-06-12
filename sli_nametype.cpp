#include "sli_nametype.h"
#include "sli_name.h"
#include "sli_token.h"

namespace sli3
{
    bool NameType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	    return false;
	return t1.data_.name_val == t2.data_.name_val;
    }

    std::ostream & NameType::print(std::ostream& out, const Token &t) const
    {
	Name myname(t.data_.name_val);
	return out << myname.toString(); 
    }

    std::ostream & LiteralType::print(std::ostream& out, const Token &t) const
    {
	Name myname(t.data_.name_val);
	return out << '/'<< myname.toString(); 
    }
}

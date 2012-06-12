#include "sli_token.h"
#include "sli_dicttype.h"
#include "sli_dictionary.h"

namespace sli3
{
    bool DictionaryType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	    return false;
	return t1.data_.dict_val == t2.data_.dict_val;
    }

    std::ostream & DictionaryType::print(std::ostream& out, const Token &t) const
    {
	assert(t.type_);
	return out << (*t.data_.dict_val); 
    }
}

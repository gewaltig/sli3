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

    std::ostream & DictionaryType::pprint(std::ostream& out, const Token &t) const
    {
	assert(t.type_);
	if (t.data_.dict_val == 0) return out << "<<null>>";
	// PostScript spec: executeonly / noaccess dicts print as
	// `--nostringval--` instead of dumping contents. Wave 2.
	if (!t.data_.dict_val->is_readable())
	    return out << "--nostringval--";
	return out << (*t.data_.dict_val);
    }

    std::ostream & DictionaryType::print(std::ostream& out, const Token &t) const
    {
	assert(t.type_);
	if (t.data_.dict_val == 0) return out << "<<null>>";
	if (!t.data_.dict_val->is_readable())
	    return out << "--nostringval--";
	return out << "<<...>>";
    }
}

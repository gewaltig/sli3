#include "sli_nametype.h"
#include "sli_name.h"
#include "sli_token.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"

namespace sli3
{

    void NameType::execute(Token &t)
    {
	sli_->EStack().top()=sli_->lookup(Name(t.data_.long_val));
    }

    bool LiteralType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	    return false;
	return t1.data_.name_val == t2.data_.name_val;
    }

    void LiteralType::serialize(Token const& t, Writer& w) const
    {
        // Names are serialized as their underlying string and re-interned
        // on load. NameType, LiteralType, SymbolType, MarkType all share
        // this payload via the LiteralType base.
        Name n(t.data_.name_val);
        w.write_string(n.toString());
    }

    void LiteralType::deserialize(Reader& r, Token& t) const
    {
        t.type_ = const_cast<LiteralType*>(this);
        std::string s = r.read_string();
        t.data_.name_val = Name(s).toIndex();
    }

    std::ostream & NameType::print(std::ostream& out, const Token &t) const
    {
	Name myname(t.data_.name_val);
	return out << myname.toString(); 
    }

    std::ostream & MarkType::print(std::ostream& out, const Token &) const
    {
	return out << "mark";
    }

    std::ostream & LiteralType::print(std::ostream& out, const Token &t) const
    {
	Name myname(t.data_.name_val);
	return out << '/'<< myname.toString(); 
    }

    std::ostream & SymbolType::print(std::ostream& out, const Token &t) const
    {
	Name myname(t.data_.name_val);
	return out << "//"<< myname.toString(); 
    }
}

#include "sli_stringtype.h"
#include "sli_interpreter.h"
#include "sli_serialize.h"

namespace sli3
{
    bool StringType::compare(const Token&t1, const Token&t2) const
    {
	if(t1.type_ != t2.type_)
	  return false;
	// Null-payload convention: two null-payload Tokens compare
	// equal; one null vs non-null compares unequal.
	if (t1.data_.string_val == 0 || t2.data_.string_val == 0)
	  return t1.data_.string_val == t2.data_.string_val;
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

    std::ostream& StringType::pprint(std::ostream& out, const Token &t) const
    {
	// PostScript / NEST 2.x convention: `=` (Token::print) emits
	// the bare content; `==` (Token::pprint) emits the literal
	// SLI syntax form `( ... )` so the output round-trips through
	// the parser.
	out << '(';
	if (t.data_.string_val != 0)
	    out << *t.data_.string_val;
	out << ')';
	return out;
    }

    void StringType::serialize(Token const& t, Writer& w) const
    {
	SLIString* s = t.data_.string_val;
	auto [id, is_new] = w.intern_object(s);
	w.write_u32(id);
	if (is_new)
	    w.write_string(s->str());
    }

    void StringType::deserialize(Reader& r, Token& t) const
    {
	uint32_t id = r.read_u32();
	SLIString* s = static_cast<SLIString*>(r.lookup_object(id));
	if (s)
	{
	    s->add_reference();
	}
	else
	{
	    s = new SLIString(r.read_string());
	    r.register_object(id, s);
	}
	t.type_ = Token::pack_type(const_cast<StringType*>(this));
	t.data_.string_val = s;
    }
}

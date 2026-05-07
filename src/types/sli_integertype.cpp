#include "sli_integertype.h"
#include "sli_serialize.h"
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

    void IntegerType::serialize(Token const& t, Writer& w) const
    {
        w.write_i64(t.data_.long_val);
    }

    void IntegerType::deserialize(Reader& r, Token& t) const
    {
        t.type_ = const_cast<IntegerType*>(this);
        t.data_.long_val = r.read_i64();
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

    void DoubleType::serialize(Token const& t, Writer& w) const
    {
        w.write_f64(t.data_.double_val);
    }

    void DoubleType::deserialize(Reader& r, Token& t) const
    {
        t.type_ = const_cast<DoubleType*>(this);
        t.data_.double_val = r.read_f64();
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

    void BoolType::serialize(Token const& t, Writer& w) const
    {
        w.write_bool(t.data_.bool_val);
    }

    void BoolType::deserialize(Reader& r, Token& t) const
    {
        t.type_ = const_cast<BoolType*>(this);
        t.data_.bool_val = r.read_bool();
    }


}

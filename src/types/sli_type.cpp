#include "sli_type.h"
#include "sli_token.h"
#include "sli_interpreter.h"
#include <cstring>
namespace sli3
{
    SLIType::SLIType(SLIInterpreter *sli, char const name[], sli_typeid id, bool exec)
	: sli_(sli),
	  name_(name),
#ifdef SLI3_NO_PTR_TAG
	  id_(id),
#endif
	  executable_(exec)
    {
#ifndef SLI3_NO_PTR_TAG
	(void)id;  // typeid lives in the pointer top byte; init_types tags it.
#endif
    }

  void SLIType::clear(Token& t) const
    {
      t.type_=0;
      // Token::value{} value-inits only the first union member;
      // memset the whole payload so every member reads as 0 (matches
      // the null-payload contract documented at sli_type.h:106-115).
      std::memset(&t.data_, 0, sizeof(t.data_));
    }

    void SLIType::deserialize(Reader&, Token& t) const
    {
        // `this` already carries the tagged pointer the caller dispatched on.
        t.type_ = const_cast<SLIType*>(this);
        // Subclasses with payload override this method. The base
        // default applies only to marker types (mark, iiterate,
        // ifor, etc.) whose payload is unused; force the union to
        // zero so callers see a defined null payload.
        std::memset(&t.data_, 0, sizeof(t.data_));
    }
 
    void SLIType::raise_type_mismatch_(unsigned int id) const
    {
	assert(sli_);
	SLIType *required=sli_->get_type(id);
	assert(required);
	throw TypeMismatch(required->name_.toString(), name_.toString());
    }

    void SLIType::execute(Token &t)
    {
	sli_->push(t);
	sli_->EStack().pop();
    }

    std::ostream & SLIType::print(std::ostream& out, const Token &) const
    {
	return out << '-' << name_ << '-';
    }

    std::ostream & SLIType::pprint(std::ostream& out, const Token & t) const
    {
	// Default pprint forwards to print. Simple types (integer,
	// double, bool, name, literal, symbol, mark) want
	// pprint == print -- the documented `=` and `==` operators
	// in sli-init.sli only diverge for aggregate/complex types
	// (procedure, string, etc.), which override pprint
	// explicitly. The previous default emitted a debug-style
	// "-typename (ptr) -" string that bled into every `==`.
	return print(out, t);
    }

 
}

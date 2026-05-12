#include "sli_functiontype.h"
#include "sli_interpreter.h"
#include "sli_function.h"
#include "sli_serialize.h"

namespace sli3
{

  bool FunctionType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.func_val == t2.data_.func_val;
  }


  std::ostream & FunctionType::pprint(std::ostream& out, const Token & t) const
  {
    if(t.data_.func_val)
      return out << '-'<< t.data_.func_val->get_name()<< '-';
    else
	return out;
  }

  std::ostream & FunctionType::print(std::ostream& out, const Token & t) const
  {
    if(t.data_.func_val)
	return out <<t.data_.func_val->get_name() ;
    else
	return out;
  }

  void FunctionType::serialize(Token const& t, Writer& w) const
  {
    // Null payload (deserialized base default, or cleared slot) is
    // valid per the null-payload convention; emit an empty name.
    if (t.data_.func_val)
      w.write_string(t.data_.func_val->get_name().toString());
    else
      w.write_string(std::string());
  }

  void FunctionType::deserialize(Reader& r, Token& t) const
  {
    std::string n = r.read_string();
    t.type_ = Token::pack_type(const_cast<FunctionType*>(this));
    if (n.empty()) { t.data_.func_val = nullptr; return; }
    // Re-resolve the operator by name in the loading interpreter.
    Token resolved;
    if (sli_->lookup(Name(n), resolved)
        && resolved.tag() == sli3::functiontype)
    {
      t.data_.func_val = resolved.data_.func_val;
    }
    else
    {
      // Operator not registered: leave func_val null. The loaded
      // Token will print/compare as a null function; executing it
      // is a no-op (see execute() above). Snapshots taken from one
      // build and loaded into a slimmer build land here.
      t.data_.func_val = nullptr;
    }
  }

  void FunctionType::execute(Token &t)
  {
    if (!t.data_.func_val) return;
    SLIFunction* fn = t.data_.func_val;
    fn->execute(sli_);
    // Axis I bundle step 3: mirror the dispatcher's main-case
    // post-check here. execute_ (plain mode) reaches this path
    // via top().execute(); the dispatcher's inline functiontype
    // case bypasses FunctionType::execute and runs its own
    // post-check. Without this, new-ABI ops dispatched via plain
    // mode leak the fn slot on the e-stack (the op doesn't
    // self-pop, and nobody pops for it).
    if (fn->uses_new_abi() && sli_->EStack().load() > 0)
    {
      Token const& post_top = sli_->EStack().top();
      if (post_top.tag() == sli3::functiontype &&
          post_top.data_.func_val == fn) {
        sli_->EStack().pop();
      }
    }
  }

}

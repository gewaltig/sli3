#include "sli_functiontype.h"
#include "sli_interpreter.h"
#include "sli_exceptions.h"
#include "sli_iostreamtype.h"
#include "sli_serialize.h"

#include <iostream>

namespace sli3
{

  bool IstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.istream_val == t2.data_.istream_val;
  }

  bool XIstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.istream_val == t2.data_.istream_val;
  }

  bool OstreamType::compare(const Token&t1, const Token&t2) const
  {
	if(t1.type_ != t2.type_)
	  return false;
	return t1.data_.ostream_val == t2.data_.ostream_val;
  }

  void XIstreamType::execute(Token &t)
  {
    // 't' is already on top of the execution stack — we just need to
    // push iparse on top of it. iparse's execute() reads the stream
    // from pick(1) and pops both itself and the stream on EOF.
    if (t.data_.istream_val)
        sli_->EStack().push(sli_->new_token<sli3::nametype, Name>(sli_->iparse_name));
    else
        throw IOError();
  }

  // Streams are not serializable — OS resources don't survive a
  // snapshot. We emit a null marker and warn so that callers know
  // a stream slot was dropped.
  static void warn_unserializable_stream(char const* dir)
  {
      std::cerr << "sli3: " << dir
                << " stream is not serializable; replaced with closed stream\n";
  }

  void IstreamType::serialize(Token const&, Writer&) const
  {
      warn_unserializable_stream("input");
  }

  void IstreamType::deserialize(Reader&, Token& t) const
  {
      t.type_ = const_cast<IstreamType*>(this);
      t.data_.istream_val = new SLIistream();  // valid()==false
  }

  void OstreamType::serialize(Token const&, Writer&) const
  {
      warn_unserializable_stream("output");
  }

  void OstreamType::deserialize(Reader&, Token& t) const
  {
      t.type_ = const_cast<OstreamType*>(this);
      t.data_.ostream_val = new SLIostream();  // valid()==false
  }

}

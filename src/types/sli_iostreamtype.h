#ifndef SLI_IOSTREAMTYPE_H
#define SLI_IOSTREAMTYPE_H
#include "sli_type.h"
#include "sli_token.h"
#include "sli_iostream.h"

namespace sli3
{

  /**
   * Output-stream type. Backing object is sli3::SLIostream — an
   * intrusive-refcounted wrapper around std::ostream*.
   *
   * Streams are not serializable — OS resources don't survive a
   * snapshot. serialize() emits a null marker (no payload); deserialize()
   * produces a Token whose SLIostream has stream_ == nullptr (a closed
   * stream). A stderr warning is emitted on serialization to surface
   * silent data loss.
   */
  class OstreamType: public SLIType
  {
  public:
    OstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type)
      {executable_=false;}

    refcount_t add_reference(Token const& t) const override
    {
      if (t.data_.ostream_val) return t.data_.ostream_val->add_reference();
      return 0;
    }

    void remove_reference(Token &t) const override
    {
      if (t.data_.ostream_val
          and t.data_.ostream_val->remove_reference() == 0)
      {
        t.type_ = 0;
        t.data_ = Token::value();
      }
    }

    void clear(Token &t) const override
    {
      remove_reference(t);
      t.data_.ostream_val = 0;
    }

    refcount_t references(Token const& t) const override
    {
      if (t.data_.ostream_val) return t.data_.ostream_val->references();
      return 0;
    }

    bool compare(const Token&t1, const Token&t2) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class IstreamType: public SLIType
  {
  public:
    IstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :SLIType(sli, name, type)
      {executable_=false;}

    refcount_t add_reference(Token const& t) const override
    {
      if (t.data_.istream_val) return t.data_.istream_val->add_reference();
      return 0;
    }

    void remove_reference(Token &t) const override
    {
      if (t.data_.istream_val
          and t.data_.istream_val->remove_reference() == 0)
      {
        t.type_ = 0;
        t.data_ = Token::value();
      }
    }

    void clear(Token &t) const override
    {
      remove_reference(t);
      t.data_.istream_val = 0;
    }

    refcount_t references(Token const& t) const override
    {
      if (t.data_.istream_val) return t.data_.istream_val->references();
      return 0;
    }

    bool compare(const Token&t1, const Token&t2) const override;
    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
  };

  class XIstreamType: public IstreamType
  {
  public:
    XIstreamType(SLIInterpreter *sli, char const name[], sli_typeid type)
      :IstreamType(sli, name, type)
      {executable_=true;}

    bool compare(const Token&t1, const Token&t2) const override;
    void execute(Token &) override;
  };



}
#endif

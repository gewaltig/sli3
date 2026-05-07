#ifndef SLI_STRING_H
#define SLI_STRING_H

#include <cstdint>
#include <ostream>
#include <string>
#include <utility>

namespace sli3
{

/**
 * Reference-counted heap-allocated std::string.
 *
 * Backing for stringtype Tokens. One heap object per shared string
 * value; the std::string member carries the data (its own SSO buffer
 * covers small strings without a second allocation on most stdlibs).
 *
 * Refcount is intrusive (single-threaded; non-atomic). The StringType
 * protocol calls add_reference() / remove_reference() in response to
 * Token copy / destruction. remove_reference() returns the new count
 * and self-deletes when it reaches zero.
 *
 * SLIString is non-copyable: sharing happens via the Token's
 * data_.string_val pointer, not via value copying.
 */
class SLIString
{
public:
    SLIString() : refs_(1) {}
    explicit SLIString(std::string s) : data_(std::move(s)), refs_(1) {}
    explicit SLIString(char const* s) : data_(s), refs_(1) {}

    SLIString(SLIString const&) = delete;
    SLIString& operator=(SLIString const&) = delete;

    uint32_t add_reference()    { return ++refs_; }
    uint32_t remove_reference()
    {
        if (--refs_ == 0)
        {
            delete this;
            return 0;
        }
        return refs_;
    }
    uint32_t references() const { return refs_; }

    std::string&       str()       { return data_; }
    std::string const& str() const { return data_; }

    // Forwards used by existing call sites (parse buffers, error
    // messages). Kept narrow on purpose.
    char const* c_str() const { return data_.c_str(); }
    size_t      size()  const { return data_.size(); }
    bool        empty() const { return data_.empty(); }

    bool operator==(SLIString const& o) const { return data_ == o.data_; }
    bool operator!=(SLIString const& o) const { return data_ != o.data_; }

    // Implicit conversion lets `*string_val` flow into call sites
    // expecting std::string& (e.g. Token::operator std::string&,
    // ostream <<, free-standing string operators).
    operator std::string&()             { return data_; }
    operator std::string const&() const { return data_; }

private:
    std::string data_;
    uint32_t    refs_;
};

inline std::ostream& operator<<(std::ostream& out, SLIString const& s)
{
    return out << s.str();
}

}  // namespace sli3

#endif

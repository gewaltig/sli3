#ifndef SLI_STRING_H
#define SLI_STRING_H

#include "sli_access.h"
#include "sli_allocator.h"

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
    // Pool allocator for the SLIString header (the std::string's
    // own SBO / heap buffer is unaffected; only the wrapper is
    // pooled). One shared pool across all SLIString instances;
    // class incomplete here, so the inline static is defined
    // below the class body.
    static Pool memory_pool_;

    static void* operator new(std::size_t sz)
    {
#ifdef SLI3_SANITIZE
        return ::operator new(sz);
#else
        if (sz != sizeof(SLIString))
            return ::operator new(sz);
        return memory_pool_.alloc();
#endif
    }

    static void operator delete(void* p, std::size_t sz) noexcept
    {
#ifdef SLI3_SANITIZE
        ::operator delete(p);
        (void)sz;
#else
        if (p == nullptr) return;
        if (sz != sizeof(SLIString)) { ::operator delete(p); return; }
        memory_pool_.free(p);
#endif
    }

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

    // PostScript-style access. Default ACCESS_UNLIMITED; monotonic
    // narrowing. String-mutating ops (put_s, append_s, replace_s, …)
    // call require_writable() at entry; readonly strings raise
    // WriteProtected on attempted mutation.
    bool     is_writable() const { return access_ == ACCESS_UNLIMITED; }
    bool     is_readable() const { return access_ <= ACCESS_READONLY; }
    uint8_t  access()      const { return access_; }
    void     set_access(uint8_t a) { if (a > access_) access_ = a; }

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
    uint8_t     access_ = ACCESS_UNLIMITED;
};

inline std::ostream& operator<<(std::ostream& out, SLIString const& s)
{
    return out << s.str();
}

inline Pool SLIString::memory_pool_(sizeof(SLIString));

}  // namespace sli3

#endif

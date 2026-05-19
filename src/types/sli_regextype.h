#ifndef SLI_REGEXTYPE_H
#define SLI_REGEXTYPE_H

#include "sli_type.h"
#include "sli_token.h"

#include <cstdint>
#include <iostream>
#include <regex.h>
#include <string>

namespace sli3
{

/**
 * Reference-counted heap-allocated POSIX regex wrapper.
 *
 * Backing for regextype Tokens. Owns one regex_t; calls regfree on
 * destruction only if compile() succeeded. The pattern string and
 * compile flags are kept so the regex can be re-compiled on
 * deserialize (regex_t is opaque — no portable way to dump its
 * internal state).
 *
 * Refcount is intrusive (single-threaded; non-atomic). The RegexType
 * protocol calls add_reference() / remove_reference() in response to
 * Token copy / destruction.
 */
class Regex
{
public:
    Regex() : refs_(1) {}
    ~Regex()
    {
        if (compiled_) regfree(&r_);
    }

    Regex(Regex const&) = delete;
    Regex& operator=(Regex const&) = delete;

    /**
     * Compile `pat` with `flags`. Re-compilation on an already-compiled
     * Regex frees the previous state first. Returns the POSIX regcomp
     * error code (0 on success).
     */
    int compile(std::string const& pat, int flags)
    {
        if (compiled_)
        {
            regfree(&r_);
            compiled_ = false;
        }
        pattern_ = pat;
        flags_ = flags;
        int e = regcomp(&r_, pat.c_str(), flags);
        compiled_ = (e == 0);
        return e;
    }

    regex_t*           posix()           { return &r_; }
    std::string const& pattern() const   { return pattern_; }
    int                flags()   const   { return flags_; }
    bool               compiled() const  { return compiled_; }

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

private:
    regex_t     r_{};
    std::string pattern_;
    int         flags_{0};
    bool        compiled_{false};
    uint32_t    refs_;
};

class RegexType : public SLIType
{
public:
    RegexType(SLIInterpreter* sli, char const name[], sli_typeid type)
        : SLIType(sli, name, type) { needs_refcount_ = true; }

    refcount_t add_reference(Token const& t) const override
    {
        if (t.data_.regex_val != 0)
            return t.data_.regex_val->add_reference();
        return 0;
    }

    void remove_reference(Token& t) const override
    {
        if (t.data_.regex_val != 0)
        {
            if (t.data_.regex_val->remove_reference() == 0)
            {
                t.type_ = 0;
                t.data_ = Token::value();
            }
        }
    }

    void clear(Token& t) const override
    {
        remove_reference(t);
        t.data_.regex_val = 0;
    }

    refcount_t references(Token const& t) const override
    {
        if (t.data_.regex_val != 0)
            return t.data_.regex_val->references();
        return 0;
    }

    bool compare(Token const& t1, Token const& t2) const override
    {
        if (t1.type_ != t2.type_) return false;
        return t1.data_.regex_val == t2.data_.regex_val;
    }

    std::ostream& print(std::ostream& out, Token const&) const override
    {
        return out << "--regex--";
    }

    std::ostream& pprint(std::ostream& out, Token const& t) const override
    {
        return print(out, t);
    }

    void serialize(Token const&, Writer&) const override;
    void deserialize(Reader&, Token&) const override;
};

}  // namespace sli3

#endif

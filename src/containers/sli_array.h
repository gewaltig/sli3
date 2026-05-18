#ifndef TOKENARRAY_H
#define TOKENARRAY_H

#include "sli_access.h"
#include "sli_allocator.h"
#include "sli_token.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <utility>
#include <vector>

namespace sli3
{

class Reader;
class SLIInterpreter;
class Writer;

/**
 * Reference-counted contiguous container of Tokens.
 *
 * Backing storage for arraytype, proceduretype, and litproceduretype
 * (per the type-economical SLIType polymorphism: same payload, three
 * execution semantics). One heap allocation per TokenArray header;
 * std::vector<Token> owns the contiguous element storage.
 *
 * Refcount is intrusive (single-threaded; non-atomic uint32_t). The
 * ArrayType protocol calls add_reference() / remove_reference() in
 * response to Token copy / destruction. remove_reference() returns the
 * new count and self-deletes when it reaches zero.
 *
 * Move/value-copy is intentionally disabled — TokenArrays are always
 * shared via Token's array_val pointer. Use the copy constructor
 * directly when a deep copy is needed (no current callers; reintroduce
 * a deep_copy() helper if needed later).
 */
class TokenArray
{
public:
    // Pool allocator for the TokenArray header (the std::vector
    // element buffer is still on the global heap; this pools only
    // the wrapper). One pool shared across all TokenArray
    // instances; bodies in sli_array.cpp.
    static Pool memory_pool_;
    static void* operator new(std::size_t sz);
    static void operator delete(void* p, std::size_t sz) noexcept;

    static size_t allocations;  // diagnostic counter

    TokenArray();
    explicit TokenArray(size_t n, Token const& fill = Token());
    TokenArray(size_t n, Token const& fill, size_t reserve_n);
    TokenArray(TokenArray const& other);
    TokenArray(TokenArray&&) = delete;
    /**
     * Replace contents with a copy of other's contents. refs_ is
     * preserved — assignment is content-replace, not identity-replace.
     */
    TokenArray& operator=(TokenArray const& other);
    TokenArray& operator=(TokenArray&&) = delete;
    ~TokenArray() = default;

    // Element access
    Token&       operator[](size_t i)             { return data_[i]; }
    Token const& operator[](size_t i) const       { return data_[i]; }
    Token const& get(long i)          const       { return data_[static_cast<size_t>(i)]; }
    Token&       get(long i)                      { return data_[static_cast<size_t>(i)]; }

    Token*       begin()                          { return data_.data(); }
    Token*       end()                            { return data_.data() + data_.size(); }
    Token const* begin() const                    { return data_.data(); }
    Token const* end()   const                    { return data_.data() + data_.size(); }

    size_t size()                  const          { return data_.size(); }
    size_t capacity()              const          { return data_.capacity(); }
    bool   empty()                 const          { return data_.empty(); }
    bool   index_is_valid(size_t i) const         { return i < data_.size(); }

    // Mutators
    void push_back(Token const& t)                { data_.push_back(t); }
    void push_back(Token&& t)                     { data_.push_back(std::move(t)); }
    void pop_back()                               { data_.pop_back(); }
    /** Ensure at least n free slots beyond current size. */
    void reserve_token(size_t n)
    {
        if (data_.capacity() - data_.size() < n)
            data_.reserve(data_.size() + n);
    }
    void clear();
    void reserve(size_t n)                        { data_.reserve(n); }
    void resize(size_t n, Token const& t = Token()) { data_.resize(n, t); }
    bool shrink();

    void erase(Token* first, Token* last);
    void erase(size_t i, size_t n);
    void erase(Token* tp)                         { erase(tp, tp + 1); }
    void reduce(Token* first, Token* last);
    void reduce(size_t i, size_t n);
    void insert(size_t i, size_t n, Token const& t);
    void insert(size_t i, Token const& t)         { insert(i, 1, t); }
    void insert(size_t i, TokenArray const& a);
    void assign(TokenArray const& a, size_t i, size_t n);
    void append(TokenArray const& a);
    void rotate(Token* first, Token* middle, Token* last);

    bool operator==(TokenArray const& o) const    { return data_ == o.data_; }
    bool operator!=(TokenArray const& o) const    { return !(*this == o); }

    // PostScript-style access. The field is on the payload so every
    // sharer observes the same state. Default ACCESS_UNLIMITED;
    // narrowing is monotonic (set_access(X) only succeeds when X is
    // stricter than the current value). Mutation sites in the C++
    // surface call require_writable() at entry; on readonly arrays
    // those raise WriteProtected.
    bool      is_writable() const                 { return access_ == ACCESS_UNLIMITED; }
    bool      is_readable() const                 { return access_ <= ACCESS_READONLY; }
    uint8_t   access()      const                 { return access_; }
    void      set_access(uint8_t a)               { if (a > access_) access_ = a; }

    // Refcount protocol — called by ArrayType::add_reference / remove_reference.
    uint32_t add_reference()                      { return ++refs_; }
    uint32_t remove_reference()
    {
        if (--refs_ == 0)
        {
            delete this;
            return 0;
        }
        return refs_;
    }
    uint32_t references() const                   { return refs_; }

    // Serialization payload — caller (ArrayType) handles intern_object;
    // these write/read the contents only.
    void serialize_body(Writer&) const;
    void deserialize_body(Reader&, SLIInterpreter&);

    void info(std::ostream&) const;
    bool valid() const                            { return true; }

private:
    std::vector<Token> data_;
    uint32_t refs_;
    uint8_t  access_ = ACCESS_UNLIMITED;
};

std::ostream& operator<<(std::ostream&, TokenArray const&);

}  // namespace sli3

#endif

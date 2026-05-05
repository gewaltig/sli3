#include "sli_array.h"

#include "sli_serialize.h"
#include "sli_token.h"

#include <algorithm>
#include <ostream>

namespace sli3
{

size_t TokenArray::allocations = 0;

TokenArray::TokenArray()
    : data_(), refs_(1)
{
    ++allocations;
}

TokenArray::TokenArray(size_t n, Token const& fill)
    : data_(n, fill), refs_(1)
{
    ++allocations;
}

TokenArray::TokenArray(size_t n, Token const& fill, size_t reserve_n)
    : refs_(1)
{
    data_.reserve(reserve_n > n ? reserve_n : n);
    if (n) data_.assign(n, fill);
    ++allocations;
}

TokenArray::TokenArray(TokenArray const& other)
    : data_(other.data_), refs_(1)
{
    ++allocations;
}

TokenArray& TokenArray::operator=(TokenArray const& other)
{
    if (this != &other)
        data_ = other.data_;  // refs_ preserved: same identity, new content
    return *this;
}

void TokenArray::clear()
{
    // swap-with-empty: drops capacity too (vector::clear() keeps it).
    std::vector<Token>().swap(data_);
}

bool TokenArray::shrink()
{
    size_t old_cap = data_.capacity();
    data_.shrink_to_fit();
    return data_.capacity() < old_cap;
}

void TokenArray::erase(Token* first, Token* last)
{
    auto i = data_.begin() + (first - data_.data());
    auto j = data_.begin() + (last  - data_.data());
    data_.erase(i, j);
}

void TokenArray::erase(size_t i, size_t n)
{
    if (i >= data_.size()) return;
    if (i + n > data_.size()) n = data_.size() - i;
    data_.erase(data_.begin() + i, data_.begin() + i + n);
}

void TokenArray::reduce(Token* first, Token* last)
{
    // Keep [first, last); erase tail then head so indices stay valid.
    size_t i = first - data_.data();
    size_t j = last  - data_.data();
    if (j < data_.size()) data_.erase(data_.begin() + j, data_.end());
    if (i > 0)            data_.erase(data_.begin(),     data_.begin() + i);
}

void TokenArray::reduce(size_t i, size_t n)
{
    if (i + n > data_.size()) n = data_.size() - i;
    reduce(data_.data() + i, data_.data() + i + n);
}

void TokenArray::insert(size_t i, size_t n, Token const& t)
{
    data_.insert(data_.begin() + i, n, t);
}

void TokenArray::insert(size_t i, TokenArray const& a)
{
    data_.insert(data_.begin() + i, a.data_.begin(), a.data_.end());
}

void TokenArray::assign(TokenArray const& a, size_t i, size_t n)
{
    data_.assign(a.data_.begin() + i, a.data_.begin() + i + n);
}

void TokenArray::append(TokenArray const& a)
{
    data_.insert(data_.end(), a.data_.begin(), a.data_.end());
}

void TokenArray::rotate(Token* first, Token* middle, Token* last)
{
    std::rotate(data_.begin() + (first  - data_.data()),
                data_.begin() + (middle - data_.data()),
                data_.begin() + (last   - data_.data()));
}

void TokenArray::serialize_body(Writer& w) const
{
    w.write_u32(static_cast<uint32_t>(data_.size()));
    for (Token const& t : data_)
        write_token(t, w);
}

void TokenArray::deserialize_body(Reader& r, SLIInterpreter& interp)
{
    uint32_t n = r.read_u32();
    data_.clear();
    data_.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        data_.push_back(read_token(r, interp));
}

void TokenArray::info(std::ostream& out) const
{
    out << "TokenArray::info  size=" << data_.size()
        << "  capacity=" << data_.capacity()
        << "  refs=" << refs_ << '\n';
}

std::ostream& operator<<(std::ostream& out, TokenArray const& a)
{
    bool first = true;
    for (Token const& t : a)
    {
        if (!first) out << ' ';
        out << t;
        first = false;
    }
    return out;
}

}  // namespace sli3

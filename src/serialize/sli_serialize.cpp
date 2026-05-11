#include "sli_serialize.h"

#include "sli_interpreter.h"
#include "sli_token.h"
#include "sli_tokenstack.h"
#include "sli_type.h"

#include <cstring>
#include <istream>
#include <ostream>
#include <stdexcept>

namespace sli3
{

// --- BinaryWriter ----------------------------------------------------------

BinaryWriter::BinaryWriter(std::ostream& out) : out_(out) {}

void BinaryWriter::write_header()
{
    write_u32(kSerializeMagic);
    write_u32(kSerializeVersion);
}

void BinaryWriter::write_u8(uint8_t v)
{
    out_.put(static_cast<char>(v));
}

void BinaryWriter::write_u16(uint16_t v)
{
    write_u8(static_cast<uint8_t>(v));
    write_u8(static_cast<uint8_t>(v >> 8));
}

void BinaryWriter::write_u32(uint32_t v)
{
    write_u8(static_cast<uint8_t>(v));
    write_u8(static_cast<uint8_t>(v >> 8));
    write_u8(static_cast<uint8_t>(v >> 16));
    write_u8(static_cast<uint8_t>(v >> 24));
}

void BinaryWriter::write_i64(int64_t v)
{
    uint64_t u = static_cast<uint64_t>(v);
    for (int i = 0; i < 8; ++i)
        write_u8(static_cast<uint8_t>(u >> (i * 8)));
}

void BinaryWriter::write_f64(double d)
{
    static_assert(sizeof(double) == 8, "non-IEEE 754 double");
    uint64_t bits;
    std::memcpy(&bits, &d, 8);
    for (int i = 0; i < 8; ++i)
        write_u8(static_cast<uint8_t>(bits >> (i * 8)));
}

void BinaryWriter::write_bytes(const void* data, size_t n)
{
    out_.write(static_cast<const char*>(data), static_cast<std::streamsize>(n));
}

std::pair<uint32_t, bool> BinaryWriter::intern_object(const void* obj)
{
    auto it = obj_ids_.find(obj);
    if (it != obj_ids_.end()) return {it->second, false};
    uint32_t id = next_id_++;
    obj_ids_.emplace(obj, id);
    return {id, true};
}

// --- BinaryReader ----------------------------------------------------------

BinaryReader::BinaryReader(std::istream& in) : in_(in) {}

void BinaryReader::read_header()
{
    uint32_t magic = read_u32();
    if (magic != kSerializeMagic)
        throw std::runtime_error("sli3 serialize: bad magic");
    uint32_t version = read_u32();
    if (version != kSerializeVersion)
        throw std::runtime_error("sli3 serialize: incompatible version");
}

uint8_t BinaryReader::read_u8()
{
    int c = in_.get();
    if (c == std::char_traits<char>::eof())
        throw std::runtime_error("sli3 serialize: unexpected EOF");
    return static_cast<uint8_t>(c);
}

uint16_t BinaryReader::read_u16()
{
    uint16_t lo = read_u8();
    uint16_t hi = read_u8();
    return static_cast<uint16_t>(lo | (hi << 8));
}

uint32_t BinaryReader::read_u32()
{
    uint32_t b0 = read_u8();
    uint32_t b1 = read_u8();
    uint32_t b2 = read_u8();
    uint32_t b3 = read_u8();
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

int64_t BinaryReader::read_i64()
{
    uint64_t u = 0;
    for (int i = 0; i < 8; ++i)
        u |= static_cast<uint64_t>(read_u8()) << (i * 8);
    return static_cast<int64_t>(u);
}

double BinaryReader::read_f64()
{
    static_assert(sizeof(double) == 8, "non-IEEE 754 double");
    uint64_t bits = 0;
    for (int i = 0; i < 8; ++i)
        bits |= static_cast<uint64_t>(read_u8()) << (i * 8);
    double d;
    std::memcpy(&d, &bits, 8);
    return d;
}

void BinaryReader::read_bytes(void* dst, size_t n)
{
    in_.read(static_cast<char*>(dst), static_cast<std::streamsize>(n));
    if (static_cast<size_t>(in_.gcount()) != n)
        throw std::runtime_error("sli3 serialize: short read");
}

void BinaryReader::register_object(uint32_t id, void* obj)
{
    objects_[id] = obj;
}

void* BinaryReader::lookup_object(uint32_t id) const
{
    auto it = objects_.find(id);
    return it == objects_.end() ? nullptr : it->second;
}

// --- Token entry points ----------------------------------------------------

void write_token(Token const& t, Writer& w)
{
    if (!t.is_valid())
    {
        w.write_u16(static_cast<uint16_t>(sli3::nulltype));
        return;
    }
    uint16_t tid = static_cast<uint16_t>(t.type_->get_typeid());
    w.write_u16(tid);
    t.type_->serialize(t, w);
}

Token read_token(Reader& r, SLIInterpreter& interp)
{
    uint16_t tid = r.read_u16();
    if (tid == sli3::nulltype) return Token();
    SLIType* type = interp.get_type(tid);
    if (!type)
        throw std::runtime_error("sli3 serialize: unknown type id");
    Token t;
    type->deserialize(r, t);
    return t;
}

void write_token_stack(TokenStack const& s, Writer& w)
{
    uint32_t n = static_cast<uint32_t>(s.load());
    w.write_u32(n);
    // pick(0) is top; pick(n-1) is bottom. Serialize bottom-first
    // so read_token_stack can push in order.
    for (uint32_t i = 0; i < n; ++i)
        write_token(s.pick(n - 1 - i), w);
}

void read_token_stack(TokenStack& s, Reader& r, SLIInterpreter& interp)
{
    uint32_t n = r.read_u32();
    s.clear();
    s.reserve(n);
    for (uint32_t i = 0; i < n; ++i)
        s.push(read_token(r, interp));
}

}  // namespace sli3

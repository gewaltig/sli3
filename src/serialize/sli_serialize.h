#ifndef SLI_SERIALIZE_H
#define SLI_SERIALIZE_H

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace sli3
{

class Token;
class SLIInterpreter;

// Wire-format constants. Append-only — see project_serialization memory.
constexpr uint32_t kSerializeMagic = 0x33494C53;  // "SLI3" little-endian
// Version history:
//   1 — initial release.
//   2 — appended PostScript access byte (ACCESS_UNLIMITED / READONLY /
//       EXECUTEONLY / NOACCESS) to TokenArray and SLIString bodies.
constexpr uint32_t kSerializeVersion = 2;

/**
 * Abstract sink for serialization. Concrete implementations:
 * - BinaryWriter: stable LE binary, primary format.
 * - TextWriter:   future addition for debugging/tests.
 *
 * Types call write_* primitives directly. Pointer-payload types call
 * intern_object() before writing the body to deduplicate shared
 * instances and break cycles.
 */
class Writer
{
public:
    virtual ~Writer() = default;

    virtual void write_u8(uint8_t) = 0;
    virtual void write_u16(uint16_t) = 0;
    virtual void write_u32(uint32_t) = 0;
    virtual void write_i64(int64_t) = 0;
    virtual void write_f64(double) = 0;
    virtual void write_bytes(const void* data, size_t n) = 0;

    void write_bool(bool b) { write_u8(b ? 1 : 0); }
    void write_string(std::string_view s)
    {
        write_u32(static_cast<uint32_t>(s.size()));
        if (!s.empty()) write_bytes(s.data(), s.size());
    }

    /**
     * First call with a given pointer returns {id, true}: caller writes
     * the object body. Subsequent calls return {id, false}: caller skips
     * the body. id is non-zero.
     */
    virtual std::pair<uint32_t, bool> intern_object(const void* obj) = 0;
};

/**
 * Abstract source for deserialization. Mirrors Writer.
 */
class Reader
{
public:
    virtual ~Reader() = default;

    virtual uint8_t  read_u8() = 0;
    virtual uint16_t read_u16() = 0;
    virtual uint32_t read_u32() = 0;
    virtual int64_t  read_i64() = 0;
    virtual double   read_f64() = 0;
    virtual void     read_bytes(void* dst, size_t n) = 0;

    bool read_bool() { return read_u8() != 0; }
    std::string read_string()
    {
        uint32_t n = read_u32();
        std::string s(n, '\0');
        if (n) read_bytes(&s[0], n);
        return s;
    }

    virtual void register_object(uint32_t id, void* obj) = 0;
    virtual void* lookup_object(uint32_t id) const = 0;
};

/**
 * Binary writer/reader: little-endian, length-prefixed strings, IEEE 754
 * doubles. The wire format does not depend on host endianness.
 */
class BinaryWriter : public Writer
{
public:
    explicit BinaryWriter(std::ostream& out);

    /** Write magic + version. Call once at the start of a stream. */
    void write_header();

    void write_u8(uint8_t) override;
    void write_u16(uint16_t) override;
    void write_u32(uint32_t) override;
    void write_i64(int64_t) override;
    void write_f64(double) override;
    void write_bytes(const void* data, size_t n) override;
    std::pair<uint32_t, bool> intern_object(const void* obj) override;

private:
    std::ostream& out_;
    std::unordered_map<const void*, uint32_t> obj_ids_;
    uint32_t next_id_ = 1;
};

class BinaryReader : public Reader
{
public:
    explicit BinaryReader(std::istream& in);

    /** Read & validate magic + version. Call once at the start. */
    void read_header();

    uint8_t  read_u8() override;
    uint16_t read_u16() override;
    uint32_t read_u32() override;
    int64_t  read_i64() override;
    double   read_f64() override;
    void     read_bytes(void* dst, size_t n) override;

    void register_object(uint32_t id, void* obj) override;
    void* lookup_object(uint32_t id) const override;

private:
    std::istream& in_;
    std::unordered_map<uint32_t, void*> objects_;
};

/**
 * Top-level Token serialization. Writes [type_id : u16] [payload].
 * Dispatches the payload to the SLIType.
 */
void write_token(Token const& t, Writer& w);
Token read_token(Reader& r, SLIInterpreter& interp);

class TokenStack;

/**
 * Serialize an entire TokenStack bottom-first. Format:
 *   [count : u32] [token_0] [token_1] ... [token_{count-1}]
 * where token_0 is the bottom-most entry.
 *
 * read_token_stack clears the destination stack and pushes the
 * elements in saved order, so depth and order are preserved.
 *
 * Used by the savestate / restorestate operators
 * (src/builtins/sli_state_ops.cpp). The Writer's object table is
 * scoped to a single save/load call, so identity sharing within
 * the snapshot is preserved (a TokenArray referenced from two
 * stack slots is serialized once and re-shared on load).
 * Identity across separate save/load calls is NOT preserved.
 */
void write_token_stack(TokenStack const& s, Writer& w);
void read_token_stack (TokenStack& s, Reader& r, SLIInterpreter& interp);

}  // namespace sli3

#endif

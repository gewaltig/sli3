#ifndef SLI_IOSTREAM_H
#define SLI_IOSTREAM_H

#include <cstdint>
#include <iosfwd>

namespace sli3
{

/**
 * Reference-counted wrapper around a std::istream pointer.
 *
 * Backing for istreamtype / xistreamtype Tokens. The wrapper either
 * owns the stream (delete on last reference) or borrows it (reference
 * to a stream owned elsewhere — e.g. std::cin).
 *
 * Refcount is intrusive (single-threaded; non-atomic). The IstreamType
 * protocol calls add_reference / remove_reference on Token copy and
 * destruction. remove_reference() returns the new count and
 * self-deletes when it reaches zero.
 *
 * SLIistream is non-copyable: sharing happens via the Token's
 * data_.istream_val pointer.
 *
 * Streams are not serializable (OS resources don't survive). On
 * save, IstreamType::serialize emits a null marker; on load,
 * deserialize produces a SLIistream with stream_ == nullptr.
 */
class SLIistream
{
public:
    SLIistream() : stream_(nullptr), owns_(false), refs_(1) {}
    /** Borrow: caller retains ownership of the stream. */
    explicit SLIistream(std::istream& s) : stream_(&s), owns_(false), refs_(1) {}
    /** Take ownership: stream is deleted when refcount drops to 0. */
    explicit SLIistream(std::istream* s) : stream_(s), owns_(true), refs_(1) {}

    SLIistream(SLIistream const&) = delete;
    SLIistream& operator=(SLIistream const&) = delete;

    ~SLIistream();

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

    std::istream* get() const   { return stream_; }
    bool          valid() const { return stream_ != nullptr; }

private:
    std::istream* stream_;
    bool          owns_;
    uint32_t      refs_;
};

class SLIostream
{
public:
    SLIostream() : stream_(nullptr), owns_(false), refs_(1) {}
    explicit SLIostream(std::ostream& s) : stream_(&s), owns_(false), refs_(1) {}
    explicit SLIostream(std::ostream* s) : stream_(s), owns_(true), refs_(1) {}

    SLIostream(SLIostream const&) = delete;
    SLIostream& operator=(SLIostream const&) = delete;

    ~SLIostream();

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

    std::ostream* get() const   { return stream_; }
    bool          valid() const { return stream_ != nullptr; }

private:
    std::ostream* stream_;
    bool          owns_;
    uint32_t      refs_;
};

}  // namespace sli3

#endif

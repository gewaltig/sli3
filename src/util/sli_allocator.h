#ifndef SLI_ALLOCATOR_H
#define SLI_ALLOCATOR_H

// Fixed-size pool (freelist) allocator. Hands out blocks of one
// configured element size, recycles freed blocks onto an intrusive
// linked list. Designed to back per-class `operator new` /
// `operator delete` on the heap-allocated payload types (Dictionary,
// TokenArray, SLIString) so that short-lived dictionaries — the
// `<< /a 1 /b 2 >> begin … end` scratch-scope pattern — don't pay a
// global-heap malloc/free per iteration.
//
// Lineage: NEST 2.20.2 sli/allocator.{h,cpp}'s `sli::pool`. Cleaned
// up for C++17, dropped the PoorMansAllocator variant (NEST-specific
// for at-scale synapse lists; not relevant here), single-threaded
// (per the project's HAVE_PTHREADS removal in Stage 2).
//
// The pool grows by allocating a chunk of `block_size` elements at a
// time; on growth the chunk is threaded into a linked list of all
// chunks, and the chunk's slots are linked onto the freelist. The
// pool destructor releases every chunk; individual `free()` calls
// just push the slot back onto the freelist.
//
// Sanitizer-aware: under SLI3_SANITIZE the pool falls back to global
// new/delete so ASan can still see every per-instance alloc/free pair
// and catch use-after-free.

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace sli3
{

class Pool
{
    struct Link { Link* next; };

    struct Chunk
    {
        Chunk(size_t bytes, Chunk* nxt)
          : mem(new char[bytes]), next(nxt) {}
        ~Chunk() { delete[] mem; }
        Chunk(Chunk const&)            = delete;
        Chunk& operator=(Chunk const&) = delete;
        char*  mem;
        Chunk* next;
    };

public:
    /**
     * @param el_size      bytes per element (padded to >= sizeof(Link)).
     * @param initial      elements per first chunk; subsequent chunks
     *                     grow by *growth_factor* each time.
     * @param growth       chunk-size multiplier (1 keeps it constant).
     */
    Pool(size_t el_size, size_t initial = 1024, size_t growth = 1)
      : el_size_(el_size < sizeof(Link) ? sizeof(Link) : el_size),
        initial_block_(initial),
        growth_factor_(growth),
        block_size_(initial) {}

    Pool(Pool const&)            = delete;
    Pool& operator=(Pool const&) = delete;

    ~Pool()
    {
        Chunk* c = chunks_;
        while (c)
        {
            Chunk* nxt = c->next;
            delete c;
            c = nxt;
        }
    }

    void* alloc()
    {
        if (head_ == nullptr)
        {
            grow(block_size_);
            block_size_ *= growth_factor_;
        }
        Link* p = head_;
        head_ = head_->next;
        ++instantiations_;
        return p;
    }

    void free(void* p)
    {
        Link* l = static_cast<Link*>(p);
        l->next = head_;
        head_ = l;
        --instantiations_;
    }

    size_t element_size()   const { return el_size_; }
    size_t instantiations() const { return instantiations_; }
    size_t capacity()       const { return total_ - instantiations_; }

private:
    void grow(size_t n)
    {
        Chunk* c = new Chunk(n * el_size_, chunks_);
        chunks_ = c;
        total_ += n;
        char* start = c->mem;
        char* last  = start + (n - 1) * el_size_;
        for (char* p = start; p < last; p += el_size_)
            reinterpret_cast<Link*>(p)->next =
                reinterpret_cast<Link*>(p + el_size_);
        reinterpret_cast<Link*>(last)->next = nullptr;
        head_ = reinterpret_cast<Link*>(start);
    }

    size_t  el_size_;
    size_t  initial_block_;
    size_t  growth_factor_;
    size_t  block_size_;
    size_t  instantiations_ = 0;
    size_t  total_          = 0;
    Chunk*  chunks_         = nullptr;
    Link*   head_           = nullptr;
};

/**
 * STL-compatible allocator that routes single-element allocations
 * through a per-T `Pool` (multi-element falls back to global new).
 * Suitable as the allocator template parameter for node-based
 * containers — std::map / std::set / std::list — where every node
 * is one rebind<_NodeT> allocate(1). Use as e.g.:
 *
 *     using NodeMap = std::map<Name, DictToken,
 *                              std::less<Name>,
 *                              PoolAllocator<std::pair<const Name,
 *                                                      DictToken>>>;
 *
 * libc++/libstdc++ will rebind to the internal node type, so the
 * pool ends up sized for the actual heap allocation. Each (T) gets
 * its own pool via the rebind chain.
 */
template <class T>
class PoolAllocator
{
public:
    using value_type = T;

    PoolAllocator() noexcept = default;
    template <class U>
    PoolAllocator(PoolAllocator<U> const&) noexcept {}

    template <class U> struct rebind { using other = PoolAllocator<U>; };

    T* allocate(std::size_t n)
    {
#ifdef SLI3_SANITIZE
        return static_cast<T*>(::operator new(n * sizeof(T)));
#else
        if (n != 1)
            return static_cast<T*>(::operator new(n * sizeof(T)));
        return static_cast<T*>(pool().alloc());
#endif
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
#ifdef SLI3_SANITIZE
        ::operator delete(p);
#else
        if (n != 1) { ::operator delete(p); return; }
        pool().free(p);
#endif
    }

    template <class U>
    bool operator==(PoolAllocator<U> const&) const noexcept { return true; }
    template <class U>
    bool operator!=(PoolAllocator<U> const&) const noexcept { return false; }

private:
    static Pool& pool()
    {
        static Pool p(sizeof(T));
        return p;
    }
};

// Helper macro: declare class-static pool + operator new/delete for
// fixed-size class T. Drop into the class body (public:) — the
// pool's storage is defined inline in the header.
//
// Under SLI3_SANITIZE the override defers to global new/delete so
// ASan can attribute UAFs precisely.
#ifdef SLI3_SANITIZE
#define SLI3_POOLED_NEW(...) /* sanitizer build: default heap */
#else
#define SLI3_POOLED_NEW(CLASS)                                          \
    static ::sli3::Pool& memory_pool_()                                 \
    {                                                                   \
        static ::sli3::Pool p(sizeof(CLASS));                           \
        return p;                                                       \
    }                                                                   \
    static void* operator new(std::size_t sz)                           \
    {                                                                   \
        if (sz != sizeof(CLASS))                                        \
            return ::operator new(sz);                                  \
        return memory_pool_().alloc();                                  \
    }                                                                   \
    static void operator delete(void* p, std::size_t sz) noexcept       \
    {                                                                   \
        if (p == nullptr) return;                                       \
        if (sz != sizeof(CLASS)) { ::operator delete(p); return; }      \
        memory_pool_().free(p);                                         \
    }
#endif

}  // namespace sli3

#endif

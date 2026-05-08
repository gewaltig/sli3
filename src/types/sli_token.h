#ifndef SLI_TOKEN_H
#define SLI_TOKEN_H
#include <iostream>
#include <string>
#include "sli_type.h"
#include "sli_exceptions.h"

/**
 * Struct Datum is the smnallest memory unit. The linked type object decides
 * how this memory unit is interpreted.
 * The interpreter manages all memory for datums.
 * Compound objects are represented as follows:
 * array: array_type 1 datum with pointer to array_rep;
 * array_rep, string_rep
 *    n_ref ! len ! [ d1,...,d_len]
 * s_expr (for lists)
 *    n_ref ! lchild ! r_child
 */

namespace sli3
{
    class SLIType;
    class TokenArray;
    class SLIFunction;
    class Dictionary;
    class SLIString;
    class SLIistream;
    class SLIostream;
    class TypeNode;

    class Token
    {
    public:
	  Token();
	  Token(SLIType *);
	  Token(const Token &);
	  Token(Token&&) noexcept;
	  ~Token();


	/**
	 * Initialize from a Token. This function assumes that the object is uninitialized.
	 */
	  Token & init(const Token &t);

	/**
	 * Initialize by assignment operation.
	 */
	  Token & operator=(const Token &t);

	/**
	 * Move-assign. Transfers payload ownership from t (whose type_ is
	 * nulled) to *this. No add_reference / remove_reference calls are
	 * made on the transferred payload — the count is unchanged.
	 *
	 * std::vector<Token> uses this during reallocation, so it must be
	 * noexcept for vector to take the strong-exception path.
	 */
	  Token & operator=(Token&&) noexcept;

	/**
	 * Move contents of t to this token. Legacy method kept for
	 * existing call sites; forwards to move-assign.
	 */
	Token & move(Token &t);

	/**
	 * Exchange the contents of t and this token.
	 */
	Token & swap(Token &t);


	/**
	 * Clear contents of token.
	 *
	 */
	void clear();

	operator int() const;
	operator long() const;
	operator double() const;
	operator bool() const;
	operator std::string&();
	operator TokenArray&();

	bool is_valid() const;

	refcount_t references() const;
	refcount_t add_reference() const;
	void remove_reference();

	bool operator==(const Token&) const;
	bool operator==(Name) const;
	bool operator==(int) const;
	bool operator==(unsigned int) const;
	bool operator==(long) const;
	bool operator==(unsigned long) const;
	bool operator==(double) const;
	bool operator==(bool) const;
	bool operator!=(const Token&) const;

	bool is_of_type(unsigned int) const;
	void require_type(unsigned int) const;

	Name get_typename() const;

	void execute();

	bool is_executable() const
	{
	  return (type_ and type_->is_executable());
	}

	std::ostream & print(std::ostream &) const;
	std::ostream & pprint(std::ostream &) const;

	/**
	 * Pointer-tagging the typeid into the top byte of type_.
	 *
	 * type_ is a SLIType* whose upper byte (bits 56-63) carries
	 * the typeid. Bottom 56 bits hold the actual address; on
	 * arm64 the CPU's Top-Byte-Ignore (TBI) feature makes
	 * `type_->X` dereferences work transparently -- we don't
	 * need to mask before access. tag() reads the typeid via a
	 * pure shift, no second memory load through SLIType::id_.
	 *
	 * On x86-64 this scheme requires LAM (Linear Address
	 * Masking) to dereference a tagged pointer; without LAM,
	 * a tag-aware accessor would mask. This is why every
	 * non-zero SLIType* that lands in type_ goes through
	 * Token::pack_type() first; users do not write raw SLIType*
	 * into type_ and read it back without masking.
	 *
	 * Sanitizers (ASAN/TSAN/MSAN) instrument loads and stores
	 * with a path that does NOT honor TBI -- they see the raw
	 * 64-bit pointer including our tag bits and treat it as an
	 * out-of-range address. Disable tagging under sanitizers;
	 * fall back to reading typeid via the pointed-to SLIType.
	 */
#if defined(__has_feature)
#  if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(memory_sanitizer)
#    define SLI3_NO_PTR_TAG 1
#  endif
#endif
#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#  define SLI3_NO_PTR_TAG 1
#endif

	static constexpr unsigned TAG_SHIFT = 56;
	static constexpr uintptr_t TAG_MASK  = uintptr_t(0xFF) << TAG_SHIFT;
	static constexpr uintptr_t ADDR_MASK = ~TAG_MASK;

	/** Return the typeid encoded in the top byte of type_. */
	unsigned tag() const
	{
#ifdef SLI3_NO_PTR_TAG
	    return type_ ? type_->get_typeid() : 0;
#else
	    return static_cast<unsigned>(
	        reinterpret_cast<uintptr_t>(type_) >> TAG_SHIFT);
#endif
	}

	/** Pack a raw SLIType* into a tagged pointer. */
	static inline SLIType* pack_type(SLIType* raw);

	SLIType *type_; //!< Tagged SLIType*; top byte = typeid (TBI). NULL when unused.
	union value
	{
	    double double_val;
	    long   long_val;
	    bool   bool_val;
	    size_t  name_val;
	    TokenArray *array_val;
	    SLIFunction *func_val;
	    Dictionary *dict_val;
	    SLIString  *string_val;
	    SLIistream *istream_val;
	    SLIostream *ostream_val;
	    TypeNode *trie_val;
	    SLIType *type_val;
	} data_;
    };

    std::ostream & operator<<(std::ostream& , const Token&);


    inline
    SLIType* Token::pack_type(SLIType* raw)
    {
#ifdef SLI3_NO_PTR_TAG
	return raw;
#else
	if (!raw) return nullptr;
	uintptr_t a = reinterpret_cast<uintptr_t>(raw);
	uintptr_t tag = static_cast<uintptr_t>(raw->get_typeid()) << TAG_SHIFT;
	return reinterpret_cast<SLIType*>(a | tag);
#endif
    }

    inline
    Token::Token()
	:type_(0), data_()
    {}

    inline
    Token::Token(SLIType *t)
	:type_(pack_type(t)), data_()
    {}

    inline
    Token::Token(Token const& s)
	:type_(s.type_),
	 data_(s.data_)
    {
	// Hot-path optimization: skip the virtual add_reference for
	// types that have no payload to refcount (integer, double,
	// bool, name, literal, symbol, mark, marker types). The
	// SLIType::needs_refcount() flag is a single byte load on
	// the type object that's already in cache, vs an indirect
	// vtable call that the dispatcher's hottest loop does
	// hundreds of millions of times per workload.
	if(type_ != 0 && type_->needs_refcount())
	    type_->add_reference(*this);
    }

    inline
    Token::Token(Token&& s) noexcept
	:type_(s.type_),
	 data_(s.data_)
    {
	  s.type_ = 0;
    }

    inline
    Token::~Token()
    {
      remove_reference();
    }

    inline
    void Token::clear()
    {
      remove_reference();
      type_=0;
    }

    inline
    Token& Token::init(const Token&t)
    {
      t.add_reference();
      type_=t.type_;
      data_=t.data_;
      return *this;
    }

    inline
    Token& Token::operator=(Token&& t) noexcept
    {
      if (this == &t) return *this;
      remove_reference();
      type_ = t.type_;
      data_ = t.data_;
      t.type_ = 0;
      return *this;
    }

    inline
    Token& Token::move( Token&t)
    {
      return *this = static_cast<Token&&>(t);
    }

    inline
    Token& Token::swap(Token&t)
    {
	  SLIType *tmp_type=type_;
	  value   tmp_data=data_;

	  type_=t.type_;
	  data_=t.data_;
	  t.type_=tmp_type;
	  t.data_=tmp_data;

	  return *this;
    }

    inline
    Token& Token::operator=(const Token &t)
    {
      // Self-assign must short-circuit: clear() decrements the
      // payload's refcount, so for a refcount-1 Token `a = a;`
      // would delete the heap object before init(t) reads from it
      // (use-after-free). A simple identity check covers the common
      // case; aliasing through a shared payload (a = b where a and
      // b independently reference the same heap object) is safe
      // because each holds its own refcount.
      if (&t == this) return *this;
      clear();
      return init(t);
    }

    inline
    bool Token::is_valid() const
    {
	  return type_ !=0;
    }

    inline
    refcount_t Token::references() const
    {
	  return (is_valid()) ? type_->references(*this):1;
    }

    inline
    refcount_t Token::add_reference() const
    {
	  if (type_ != 0 && type_->needs_refcount())
	      return type_->add_reference(*this);
	  return 1;
    }

    inline
    void Token::remove_reference()
    {
	  if (type_ != 0 && type_->needs_refcount())
	    type_->remove_reference(*this);
    }

    inline
    Token::operator int() const
    {
	  require_type(sli3::integertype);
	  return data_.long_val;
    }


    inline
    Token::operator long() const
    {
	  require_type(sli3::integertype);
	  return data_.long_val;
    }

  inline
  Token::operator double() const
  {
	require_type(sli3::doubletype);
	return data_.double_val;
  }

  inline
  Token::operator bool() const
  {
	require_type(sli3::booltype);
	return data_.bool_val;
  }


    inline
    bool Token::is_of_type(unsigned int id) const
    {
	return type_ and type_->is_type(id);
    }

    inline
      void Token::require_type(unsigned int id) const
    {
      if(not type_)
	throw InvalidToken();
      type_->require_type(id);
    }

    inline
    void Token::execute()
    {
      if(type_)
	{
	  type_->execute(*this);
	  return;
	}
      throw InvalidToken();
    }


}

#endif

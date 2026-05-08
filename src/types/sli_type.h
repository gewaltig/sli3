#ifndef SLI_TYPE_H
#include <string>
#define SLI_TYPE_H
#include "sli_name.h"

namespace sli3
{

  /**
   * Class SLIType provides the virtual interface to all Datum operations. 
   * It carries a pointer to the interpreter to be able to handle storage.
   * The most common operations are provided by a fat interface with default implementations
   * that throw the appropriate error.
   */
 
  /**
   * These are the pre-defined type identifiers.
   * Each enum value has an associated SLIType class.
   */
    
  enum sli_typeid
  {
    nulltype=0,  //!< used for unassigned type_id values
    anytype,     //!< Used for typechecking only
    integertype,
    doubletype,
    booltype,
    literaltype,
    marktype,
    stringtype,
    arraytype,
    dictionarytype,
    nametype,
    litproceduretype, 
    trietype,         //!< Type Trie
    functiontype,
    proceduretype,
    istreamtype,
    xistreamtype,
    ostreamtype,
    symboltype, //!< last datatype system operators follow
    iiteratetype,
    irepeattype,
    ifortype,
    iforalltype,
    nooptype,
    quittype,
    num_sli_types, //!< Number of builtin types. User type ids start here
    intvectortype,
    doublevectortype
  };

  typedef unsigned int refcount_t;

  class SLIInterpreter;
  class Token;
  class Reader;
  class Writer;

  class SLIType
  {

  public:

    SLIType(SLIInterpreter *, char const[], sli_typeid, bool=true);
    virtual ~SLIType(){}

    /**
     * Serialize the Token's payload only — the caller writes the type-id
     * tag. Default implementation is a no-op (suitable for marker types
     * with no payload: marktype, iiteratetype, irepeattype, etc.).
     * Pointer-payload types must use the Writer's object table to
     * deduplicate shared instances and break cycles. See
     * project_serialization design notes.
     */
    virtual void serialize(Token const&, Writer&) const {}

    /**
     * Deserialize the Token's payload. The caller has already read the
     * type-id and resolved this SLIType. Sets t.type_ to this and reads
     * the payload from the Reader.
     */
    virtual void deserialize(Reader&, Token& t) const;

    virtual refcount_t add_reference(Token const&) const
    {
      return 1;
    }

    /**
     * Remove the reference count of the token.
     * This function leaves the token untouched.
     */
    virtual void remove_reference(Token &) const
    {}

    /**
     * Clear contents of the token.
     * If the token is a pointer type,
     * the remove_reference() is called
     * on the pointee.
     * This is implemented by the respective type classes.
     * All other token are reset to their default values.
     *
     * Null-payload convention (StringType, ArrayType, DictionaryType,
     * IstreamType, OstreamType, TrieType): a Token tagged with one
     * of these typeids whose payload pointer is nullptr is a valid
     * state. add_reference and references return 0; remove_reference
     * is a no-op; compare equates null payloads (and treats null vs
     * non-null as unequal); print emits a "null" placeholder. Nothing
     * derefs the payload pointer without first checking it. This
     * state is reachable via Token::value() zero-fill on a freshly
     * cleared slot and via SLIType::deserialize() (the base default)
     * for any subclass whose serialize override is missing.
     */
    virtual void clear(Token& t) const;
    
    virtual refcount_t references(const Token&) const
    {
      return 1;
    }

    /**
     * This action is performed on the token.
     * Derived types will implement their own semantics here.
     * execute(t) expects that t resides on top of the execution stack.
     */    
    virtual void execute(Token&);
    
    Name get_typename() const
    {
      return name_;
    }
    

    bool is_type(unsigned int id) const
    {
      return id_ == id;
    }

    unsigned int get_typeid() const
    {
      return id_;
    }

    void require_type(unsigned int id)
    {
      if(id != id_)
	raise_type_mismatch_(id);
    }

    bool is_executable() const
    {
      return executable_;
    }

    SLIInterpreter *get_interpreter() const
    {
      return sli_;
    }

    /**
     * Hot-path optimization: every Token copy and destruction checks
     * whether to call the virtual add_reference / remove_reference.
     * For scalar types (integer, double, bool, name, literal,
     * symbol, mark, marker types) the answer is always "no" and the
     * virtual is a wasted indirect branch in the dispatcher's hottest
     * loop. Pointer-payload subclasses (StringType, ArrayType,
     * DictionaryType, IstreamType, OstreamType, TrieType) override
     * the constructor to set this true.
     */
    bool needs_refcount() const { return needs_refcount_; }

    virtual bool compare(Token const &t1, Token const& t2) const=0;

    virtual std::ostream& print(std::ostream &, const Token &) const;
    virtual std::ostream& pprint(std::ostream &out, const Token &t) const;

 protected:
    void raise_type_mismatch_(unsigned int) const;

    SLIInterpreter *sli_;
    Name name_;
    unsigned int id_;
    bool executable_;
    bool needs_refcount_ = false;
  };
  
  
}
#endif

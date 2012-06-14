#include <string>
#ifndef SLI_TYPE_H
#define SLI_TYPE_H

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
    integertype=0,
    doubletype,
    booltype,
    literaltype,
    nametype,
    symboltype,
    stringtype,
    arraytype,
    litproceduretype,
    proceduretype,
    dictionarytype,
    functiontype,
    trietype,
    istreamtype,
    xistreamtype,
    ostreamtype,
    intvectortype,
    doublevectortype,
    iteratortype,
    num_sli_types
  };

  typedef unsigned int refcount_t;

  class SLIInterpreter;
  class Token;

  class SLIType
  {
    
  public:
    
    SLIType(SLIInterpreter *, char const[], sli_typeid, bool=true);
    
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
     */ 
    virtual void clear(Token& t) const;
    
    virtual refcount_t references(const Token&) const
    {
      return 1;
    }
    
    virtual void execute(Token&)
    {} // Default action.
    
    std::string const& get_typename() const
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

    virtual bool compare(Token const &t1, Token const& t2) const=0;
    virtual std::ostream& print(std::ostream &, const Token &) const=0;

    virtual std::ostream& pprint(std::ostream &out, const Token &t) const
      {
	return print(out, t);
      }

 protected:
    void raise_type_mismatch_(unsigned int) const;

    SLIInterpreter *sli_;
    std::string name_;
    unsigned int id_;
    bool executable_;
  };
  
  
}
#endif

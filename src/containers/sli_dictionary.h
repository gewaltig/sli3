/*
 *  dict.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#ifndef SLI_DICTIONARY_H
#define SLI_DICTIONARY_H
/* 
    SLI's dictionary class
*/

#include "sli_name.h"
#include "sli_token.h"
#include "sli_exceptions.h"

#include <map>
namespace sli3
{

/**
 * Inside dictionaries we keep track of access. That's why we wrap the Token into a class with an access control flag.
 */
  class DictToken: public Token
  {
  public:
      DictToken()
	  :Token(),
	   access_flag_(false)
	  {}

      DictToken(const DictToken& t)
      :Token(t),
       access_flag_(t.access_flag_)
	  {}

      DictToken(const Token &t):
      Token(t),
      access_flag_(false)
	  {}

      DictToken& operator=(const DictToken&t)
	  {
	      access_flag_=t.access_flag_;
	      Token::operator=(t);
	      return *this;
	  }

      bool accessed() const
	  {
	      return access_flag_;
	  }
      
      void clear_access_flag() const
	  {
	      access_flag_=false;
	  }
      
      void set_access_flag() const
	  {
	      access_flag_=true;
	  }

  private:
      mutable
	bool access_flag_;
  };

  //typedef  std::map<Name, DictToken, std::less<Name> > TokenMap;
typedef  std::map<Name, DictToken, std::less<Name> > TokenMap;

inline bool operator==(const TokenMap & x, const TokenMap &y)
{
  return (x.size() == y.size()) && equal(x.begin(), x.end(), y.begin());
}

/** A class that associates names and tokens.
 *  @ingroup TokenHandling
 */
class Dictionary :private TokenMap
{
  /**
   * Helper class for lexicographical sorting of dictionary entries.
   * Provides comparison operator for ascending, case-insensitive 
   * lexicographical ordering.
   * @see This is a simplified version of the class presented in 
   * N.M.Josuttis, The C++ Standard Library, Addison Wesley, 1999,
   * ch. 6.6.6.
   */    
  class DictItemLexicalOrder {
  private:
    static bool nocase_compare(char c1, char c2);

  public:
    bool operator() (const std::pair<Name, DictToken>& lhs, 
		     const std::pair<Name, DictToken>& rhs) const
      {
	const std::string& ls = lhs.first.toString();
	const std::string& rs = rhs.first.toString();

	return std::lexicographical_compare(ls.begin(), ls.end(),
					    rs.begin(), rs.end(),
					    nocase_compare);
      }
  };
  
public:
 Dictionary():
     TokenMap(),
     references_(1), 
     refs_on_dictstack_(0)
    {}

 Dictionary(const Dictionary &d) 
   : TokenMap(d), 
   references_(1), 
   refs_on_dictstack_(0) {}
  ~Dictionary();
  
  refcount_t add_reference() const
  {
    return ++references_;
  }

  refcount_t references() const
  {
    return references_;
  }

  refcount_t remove_reference()
  {
    if(--references_ ==0)
      {
	delete this;
	return 0;
      }
    return references_;
  }

  using TokenMap::erase;
  using TokenMap::size;
  using TokenMap::begin;
  using TokenMap::end;
  using TokenMap::iterator;
  using TokenMap::find;

  void clear();

  /**
   * Lookup and return Token with given name in dictionary.
   * @note The token returned should @b always  be stored as a 
   *       <tt>const \&</tt>, so that the control flag for 
   *       dictionary read-out is set on the Token in the dictionary,
   *       not its copy.  
   */
  bool lookup(Name const &, Token &);
  DictToken & lookup(Name const & n); //throws UndefinedName
  bool known( Name const & ) const;
  
  DictToken & insert(Name const & , Token const &t);

  //! Remove entry from dictionary
  void remove(Name const & n);  

  const DictToken& operator[](Name const &) const;
  DictToken& operator[](Name const & );
  const DictToken& operator[](const char*) const;
  DictToken& operator[](const char *);
  
  bool empty(void) const { return TokenMap::empty(); }
      
  void info(std::ostream &) const;
  
  bool operator==(const Dictionary &d) const { return sli3::operator==(*this, d); }

  // add_dict / remove_dict (sli3::Dictionary -> sli3::Dictionary
  // copies via interpreter lookup) were declared here but the cpp
  // implementations have been commented out for years -- they
  // referenced the legacy DictionaryDatum API. Removed as dead code
  // in Stage 2.8; reintroduce when there is an actual caller.

  /**
   * Clear access flags on all dictionary elements.
   * Flags for nested dictionaries are cleared recursively.
   * @see all_accessed()
   */
  void clear_access_flags();
  
  /**
   * Check whether all elements have been accessed.
   * Checks nested dictionaries recursively.
   * @param std::string& contains string with names of non-accessed entries
   * @returns true if all dictionary elements have been accessed
   * @note this is just a wrapper, all_accessed_() does the work, hides recursion
   * @see clear_access_flags(), all_accessed_()
   */
  bool all_accessed(std::string& missed) const { return all_accessed_(missed); }

  friend std::ostream & operator<<(std::ostream &, const Dictionary &);
  
  
  /** 
   * Constant iterator for dictionary.
   * Dictionary inherits privately from std::map to hide implementation
   * details. To allow for inspection of all elements in a dictionary,
   * we export the constant iterator type and begin() and end() methods.
   */  
  typedef TokenMap::const_iterator const_iterator;
  
  /** 
   * First element in dictionary.
   * Dictionary inherits privately from std::map to hide implementation
   * details. To allow for inspection of all elements in a dictionary,
   * we export the constant iterator type and begin() and end() methods.
   */  
  //  const_iterator begin() const;

  /** 
   * One-past-last element in dictionary.
   * Dictionary inherits privately from std::map to hide implementation
   * details. To allow for inspection of all elements in a dictionary,
   * we export the constant iterator type and begin() and end() methods.
   */  
  //const_iterator end() const;

  /**
   *
   */
  void initialize_property_array(Name const & propname);
  
  /**
   * This function is called when a dictionary is pushed to the dictionary stack.
   * The dictioray stack must keep track about which dictioraries are on the dictionary stack. 
   * If a dictionary is modified and it is on the dictionary stack, the cache of the dictionary stack must
   * be adjusted. This is e.g. the case for the systemdict or the errordict.
   */ 
  void add_dictstack_reference()
  {
    ++refs_on_dictstack_;
  }

  /**
   * This function is called when the dictionary is popped from the dictionary stack.
   */
  void remove_dictstack_reference()
  {
    --refs_on_dictstack_;
  }

  /**
   * Returns true, if the dictionary has references on the dictionary stack.
   */
  bool is_on_dictstack() const
  {
    return refs_on_dictstack_ >0;
  }

 private:
  /**
   * Worker function checking whether all elements have been accessed.
   * Checks nested dictionaries recursively.
   * @param std::string& contains string with names of non-accessed entries
   * @param std::string prefix for nested dictionary entries, built during recursion
   * @returns true if all dictionary elements have been accessed
   * @note this is just the worker for all_accessed()
   * @see clear_access_flags(), all_accessed()
   */
  bool all_accessed_(std::string&, std::string prefix = std::string()) const;
  
  mutable
    refcount_t references_;
  refcount_t refs_on_dictstack_;

};

 inline
   bool Dictionary::lookup(Name const & n, Token &result)
   {
     TokenMap::iterator where = find(n);
     if(where != end())
       {
	 // Set the access flag on the STORED DictToken before
	 // copying out. Without this, the bool-returning overload
	 // silently defeats access tracking (the flag would be set
	 // on the result copy, not on the dict entry).
	 (*where).second.set_access_flag();
	 result= (*where).second;
	 return true;
       }
     else
       return false;
   }

 inline
   DictToken& Dictionary::lookup(Name const & n)
 {
   // Return-by-reference form: caller sees the stored DictToken
   // directly and is responsible for calling set_access_flag()
   // on it after the read. Auto-setting here would also dirty
   // .accessed() queries (see test_dictionary.cpp -- queries
   // must be observable as not-yet-accessed).
   TokenMap::iterator where = find(n);
   if(where != end())
     return (*where).second;
   else
     throw UndefinedName(n.toString());
 }
 
inline
bool Dictionary::known(Name const & n) const
{
  TokenMap::const_iterator where = find(n);
  if(where != end())
    return true;
  else
    return false;
}

inline
DictToken& Dictionary::insert(Name const & n, Token const &t)  
{ 
    return TokenMap::operator[](n) = DictToken(t); 
}


inline
const DictToken& Dictionary::operator[](Name const & n) const
{      
  TokenMap::const_iterator where = find(n);
  if(where != end())
    return (*where).second;
  else
    throw UndefinedName(n.toString());
}


inline
DictToken& Dictionary::operator[](Name const& n)
{  
  return TokenMap::operator[](n);
}


}
#endif

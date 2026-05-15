/*
 *  dictstack.h
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

#ifndef SLI_DICTSTACK_H
#define SLI_DICTSTACK_H
/*
    SLI's dictionary stack
*/
#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include <list>


/***************************************************************

Problems:

- is ist better to uses dictionaries as references to common
  objects like in PS. What is the exact meaning of undef and
  where in our current situation (read RedBook).
- more efficient implementation exploiting
  the name ids (documented elsewhere).



    History:
            (1) using list<Dictionary>
               MD, 23.6.1, Freiburg
            (0) first version (single dictionary)
               MOG, MD, June 1997, Freiburg
***************************************************************
*/

/**
 * The macro DICTSTACK_CACHE switches on two caches:
 * 1. cache_, global cache for the dictionary stack
 * 2. basecache_, a cache for the system dictionary
 * These caches are direct lookup table with one entry per name.
 * They work as follows:
 * If a name is looked up, it is looked up in the cache.
 * If the cache does not have an entry, the dictionary stack is searched and
 * the name/token combination is added to the cache.
 */

namespace sli3
{
class DictionaryStack
{
private:
    std::list<Dictionary *> d;
    // base_ is the bottom-of-stack dict (system dict in normal use).
    // set_basedict() pins it after the first push. Until then it
    // stays null; baselookup / basedef raise rather than segfault.
    Dictionary *base_ = nullptr;
    std::vector<Token *> cache_;
    std::vector<Token *> basecache_;

    // Single search core. Returns a pointer into the owning dict's
    // TokenMap slot (nullptr on miss). Both public lookup variants
    // delegate here; do NOT replace one variant by calling the other,
    // see the comment above bool lookup() below.
    Token *find_(Name const & n)
    {
      Name::handle_t key = n.toIndex();
      if (key < cache_.size())
      {
        if (Token *ct = cache_[key])
          return ct;
      }
      for (Dictionary *dict : d)
      {
        TokenMap::iterator where = dict->find(n);
        if (where != dict->end())
        {
          cache_token(n, &(where->second));
          return &(where->second);
        }
      }
      return nullptr;
    }

public:
  DictionaryStack();
  DictionaryStack(const DictionaryStack&);
  ~DictionaryStack();


  /**
   * Add a token to the cache.
   */
  void cache_token( Name const & n, Token *result)
  {
    Name::handle_t key=n.toIndex();
    if (key>=cache_.size())
      cache_.resize(Name::num_handles()+100,0);
    cache_[key]= result;
  }

  void basecache_token(Name const & n, Token *result)
  {
    Name::handle_t key=n.toIndex();
    if (key>=basecache_.size())
      basecache_.resize(Name::num_handles()+100,0);
    basecache_[key]= result;
  }

  /**
   * Clear a name from the cache.
   * This function should be called in each def variant.
   */
  void clear_token_from_cache(Name const & n)
  {
    Name::handle_t key=n.toIndex();
    if(key<cache_.size())
      cache_[key]=0;
  }

  void clear_token_from_basecache(Name const & n)
  {
    Name::handle_t key=n.toIndex();
    if(key<basecache_.size())
      basecache_[key]=0;
  }

  void clear_dict_from_cache(Dictionary *dict)
  {
    for(TokenMap::iterator i = dict->begin(); i != dict->end(); ++i)
      clear_token_from_cache(i->first);
  }


  /** Clear the entire cache.
   * This should be  called whenever a dictionary is pushed or poped.
   * Alternative, one could just clear the names from the moved dictionary.
   */
  void clear_cache()
  {
    const size_t cache_size=cache_.size();

    for(size_t i=0; i< cache_size; ++i)
      cache_[i]=0;

    // basecache_ holds raw Token* into base_'s map; once we clear
    // the dictstack (or change base_), those pointers dangle.
    const size_t basecache_size=basecache_.size();
    for(size_t i=0; i< basecache_size; ++i)
      basecache_[i]=0;
  }

  // Two lookup variants, intentionally distinct. Both delegate to
  // find_() so the search logic exists once.
  //
  //   bool lookup(Name, Token&) — COPIES the found token into the
  //     caller's slot; returns false on miss. Use when absence is a
  //     normal outcome (known(), conditional resolution, error
  //     recovery) or when the caller wants an owned copy.
  //
  //   Token& lookup(Name)      — Returns a reference to the map slot
  //     itself; throws UndefinedName on miss. Use on the hot
  //     dispatch path (nametype execution, trie eval) where misses
  //     are exceptional and avoiding the token copy matters.
  //
  // Do NOT collapse one into the other by try/catching UndefinedName
  // around the throwing variant: the bool form is the engine of
  // known() and is called where misses are expected, so throwing on
  // absence would turn a list-walk into a stack unwind on every
  // negative answer.
  bool lookup(Name const & n, Token &result)
  {
    if (Token *p = find_(n))
    {
      result = *p;
      return true;
    }
    return false;
  }

  Token& lookup(Name const & n)
  {
    if (Token *p = find_(n))
      return *p;
    throw UndefinedName(n.toString());
  }

  bool known(Name const & n)
  {
    return find_(n) != nullptr;
  }

  /** Lookup a name searching only the bottom level dictionary.
   *  If the Name is not found,
   *  @a VoidToken is returned.
   */
  Token& baselookup(Name const & n) // lookup in a specified
  {                                           // base dictionary
    if (base_ == nullptr)
      throw UndefinedName("base dictionary not set");
    Name::handle_t key=n.toIndex();
    if (key<basecache_.size())
      {
	Token *ct=basecache_[key];
	if(ct)
	    return *ct;
      }
    TokenMap::iterator where =base_->find(n);

    if ( where != base_->end() )
      {
	cache_token(n, &(where->second)); // Update the cache
	basecache_token(n, &(where->second)); // and the basecache
	return where->second;
      }
    throw UndefinedName(n.toString());
  }


   //
   // def and def_move operate on the top level dictionary.
   // undef is not defined for the dictionary stack.

  /** Bind a Token to a Name in the top level dictionary.
   *  The Token is copied.
   */
  void def(Name const & , const Token &);

  /** Unbind a previously defined Name from the top dictionary.
   *  PostScript spec: undef removes the binding from the topmost
   *  (current) dict only, leaving any shadowed bindings in lower
   *  dicts intact. Throws UndefinedName if the top dict has no
   *  such key.
   */
  void undef(Name const & );

  /** Bind a Token to a Name in the bottom level dictionary.
   *  The Token is copied.
   */
  void basedef(Name const & n, const Token &t);

  /**
   * This function must be called once to initialize the systemdict cache.
   */
  void set_basedict();


  /** Find the dictionary on the stack that holds the given Name.
   *  Returns the containing Dictionary* on success (top-down so
   *  the first shadowing binding wins, matching PostScript
   *  `where` semantics) or nullptr if not found. Bypasses the
   *  lookup cache — the cache stores the Token slot but not
   *  which dict owns it. Caller wraps the result in a Token.
   */
  Dictionary * where(Name const & n) const
  {
    for (std::list<Dictionary *>::const_iterator it = d.begin();
         it != d.end(); ++it)
    {
      if ((*it)->known(n))
        return *it;
    }
    return nullptr;
  }

  void pop(void);


  Dictionary * top() const;
  void push(Token const &);

  void clear(void);
  void toArray(SLIInterpreter &, TokenArray &) const;

    //
    // number of dictionaries currently on the stack
    //
  size_t size(void) const;


   //
   // info for debugging purposes.
   // calls info(ostream&) for all dictionaries
   //
  void info(std::ostream&) const;
  void top_info(std::ostream &) const; // calls info of top dictionary
  const DictionaryStack& operator=(const DictionaryStack&);
};

inline
void DictionaryStack::def(Name const & n, const Token &t)
{
  //
  // insert (n,t) in top level dictionary
  // dictionary stack must contain at least one dictionary
  // VoidToken is an illegal value for t.
  //
  cache_token(n,&((*d.begin())->insert(n,t)));
  (**d.begin())[n]=t;
}


}
#endif

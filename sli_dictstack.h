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
    Dictionary *base_;
    std::vector<Token *> cache_;
    std::vector<Token *> basecache_;

public:
  DictionaryStack();
  DictionaryStack(const DictionaryStack&);
  ~DictionaryStack();

    
  /**
   * Add a token to the cache.
   */
  void cache_token( Name n, Token *result)
  {
    Name::handle_t key=n.toIndex();
    if (key>=cache_.size())
      cache_.resize(Name::num_handles()+100,0);
    cache_[key]= result;
  }

  void basecache_token(Name n, Token *result)
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
  void clear_token_from_cache(Name n)
  {
    Name::handle_t key=n.toIndex();
    if(key<cache_.size())
      cache_[key]=0;
  }

  void clear_token_from_basecache(Name n)
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
  }

  bool lookup(Name n, Token &result)
  { 
    Name::handle_t key=n.toIndex();
    if (key<cache_.size())
      {
    	Token *ct=cache_[key];
    	if(ct)
	  {
	    result=*ct;
	    return true;
	  }
      }

    std::list<Dictionary *>::const_iterator i=d.begin();
    
    while (i!=d.end())
      {
	TokenMap::iterator where =(*i)->find(n);
	if(where!=(*i)->end())
	  {
	    cache_token(n,&(where->second)); // Update the cache 
	    result= where->second;
	    return true;
	  }
	++i;
      }
    return false;
  }

  Token& lookup(Name n)
  { 
    Name::handle_t key=n.toIndex();
    if (key<cache_.size())
      {
    	Token *ct=cache_[key];
    	if(ct)
	  return *ct;
      }
    
    std::list<Dictionary *>::const_iterator i=d.begin();
    
    while (i!=d.end())
      {
	TokenMap::iterator where =(*i)->find(n);
	if(where!=(*i)->end())
	  {
	    cache_token(n,&(where->second)); // Update the cache 
	    return where->second;
	  }
	++i;
      }
    throw UndefinedName(n.toString());
  }
  
  bool known(Name n)
  {
    Token result;
    return lookup(n,result);
  }

  /** Lookup a name searching only the bottom level dictionary.
   *  If the Name is not found,
   *  @a VoidToken is returned.
   */
  Token& baselookup(Name n) // lookup in a specified
  {                                           // base dictionary
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
  void def(Name , const Token &);

  /** Unbind a previously defined Name from its token. Seach in all dictionaries.
   */
  void undef(Name );

  /** Bind a Token to a Name in the bottom level dictionary.
   *  The Token is copied.
   */
  void basedef(Name n, const Token &t);

  /**
   * This function must be called once to initialize the systemdict cache.
   */
  void set_basedict();


  bool where(Name, Token&);
    
  void pop(void);


  Dictionary * top() const; 
  void push(Token);
   
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
void DictionaryStack::def(Name n, const Token &t)
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






/*
 *  dictstack.cc
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
#include "sli_interpreter.h"
#include "sli_dictstack.h"
#include "sli_dictionary.h"
#include <iostream>

namespace sli3
{

DictionaryStack::DictionaryStack( )
{    
}

DictionaryStack::DictionaryStack(const DictionaryStack &ds )
  :  d(ds.d)
{ 
}

DictionaryStack::~DictionaryStack()
{
  // We have to clear the dictionary before we delete it, otherwise the
  // dictionary references will prevent proper deletion.
    for(std::list<Dictionary *>::iterator i=d.begin(); i != d.end(); ++i)
      (*i)->clear();
}

void DictionaryStack::undef(Name const & n)
{
    
    size_t num_erased = 0;
    for (std::list<Dictionary *>::iterator it = d.begin();
	 it != d.end();
	 it++)    
	num_erased += (*it)->erase(n);    
    
    if (num_erased == 0)
	throw UndefinedName(n.toString());
#ifdef DICTSTACK_CACHE
    clear_token_from_cache(n);
    clear_token_from_basecache(n);
#endif
}

void DictionaryStack::basedef( Name const & n, const Token &t)
{
  //
  // insert (n,t) in bottom level dictionary
  // dictionary stack must contain at least one dictionary
  // VoidToken is an illegal value for t.
#ifdef DICTSTACK_CACHE
    clear_token_from_cache(n);
    basecache_token(n,&(base_->insert(n,t)));
#endif
#ifndef DICTSTACK_CACHE
    (*base_)[n]=t;
#endif
}



void DictionaryStack::pop(void)
{
  //
  // remove top dictionary from stack
  // dictionary stack must contain at least one dictionary
  //

  Dictionary *top = *(d.begin());

  // Cache invalidation. Cached pointers into `top`'s entries become
  // dangling as soon as `top` is gone (and once we drop our reference,
  // it may actually be deleted). Same rationale as push: cache_token
  // is unconditional, so invalidation must be too.
  clear_dict_from_cache(top);

#ifdef DICTSTACK_CACHE
    top->remove_dictstack_reference();
#endif
    d.pop_front();

    // Pair with the add_reference in push(): now that the dict is no
    // longer on the dictstack, drop the reference. May delete the dict
    // if no Token still holds it.
    top->remove_reference();
}

void DictionaryStack::clear(void)
{ 
    d.erase(d.begin(),d.end());
#ifdef DICTSTACK_CACHE
    clear_cache();
#endif
}


Dictionary *DictionaryStack::top() const
{
  return *d.begin();
}

void DictionaryStack::toArray(SLIInterpreter &sli, TokenArray &ta) const
{
  //
  // create a copy of the top level dictionary
  // and move it into Token t.
  // new should throw an exception if it fails
  //

  ta.clear();
  
  std::list<Dictionary *>::const_reverse_iterator i(d.rbegin());

  while (i!=d.rend())
  {
    Token dicttoken;
    dicttoken.type_=sli.get_type(sli3::dictionarytype);
    dicttoken.data_.dict_val=*i;
    ta.push_back(dicttoken);
   ++i;
  }
}
  
void DictionaryStack::push(Token dicttoken)
{
  assert(dicttoken.is_of_type(sli3::dictionarytype));
  Dictionary *dict=dicttoken.data_.dict_val;
  //
  // extract Dictionary from Token envelope
  // and push it on top of the stack.
  // a non dictionary datum at this point is a program bug.
  //

  // The dictstack stores raw Dictionary* but Token-side refcounting
  // is what keeps Dictionaries alive. Bump the count so that destruction
  // of the source Token (or any other Token holding this dict) doesn't
  // delete the dict out from under us. The matching decrement is in
  // DictionaryStack::pop().
  dict->add_reference();

  // The name cache (cache_[]) holds raw Token* pointers into dictionary
  // entries. Names already cached for a dict that's not yet on the stack
  // would shadow same-named bindings from the new top dict. Pushing a
  // dict therefore always invalidates any cached entries that share its
  // keys. (cache_token() in lookup is called regardless of the
  // DICTSTACK_CACHE macro, so invalidation must be too.)
  clear_dict_from_cache(dict);

#ifdef DICTSTACK_CACHE
    dict->add_dictstack_reference();
#endif

    d.push_front(dict);
}

void DictionaryStack::set_basedict()
{
    base_= *(--d.end()); // Cache base dictionary
}

size_t DictionaryStack::size(void) const
{
  //
  // return number of dictionaries on stack
  //

  return d.size();
}


void DictionaryStack::info(std::ostream& o) const
{
//  for_each(d.rbegin(), d.rend(), bind_2nd(mem_fun(&Dictionary::info),o));

  std::list<Dictionary *>::const_reverse_iterator i(d.rbegin());

  o << "DictionaryStack::info" << std::endl;
  o << "Size = " << d.size() << std::endl;
  while (i!=d.rend())
  {
    (*i)->info(o);
    ++i;
  }
}

void DictionaryStack::top_info(std::ostream& o) const
{
    (*d.begin())->info(o);
}
 
const DictionaryStack& DictionaryStack::operator=(const DictionaryStack& ds)
{
  if(&ds != this)
  {
    d=ds.d;
#ifdef DICTSTACK_CACHE
    cache_=ds.cache_;
#endif
  }
  return *this;
}
}

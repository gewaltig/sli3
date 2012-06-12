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

void DictionaryStack::undef(Name n)
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

void DictionaryStack::basedef( Name n, const Token &t)
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


void DictionaryStack::basedef_move( Name n, Token &t)
{
#ifdef DICTSTACK_CACHE
    clear_token_from_cache(n);
    basecache_token(n,&(base_->insert_move(n,t)));
#endif
#ifndef DICTSTACK_CACHE
    base_->insert_move(n, t);
#endif
}


void DictionaryStack::pop(void)
{ 
  //
  // remove top dictionary from stack
  // dictionary stack must contain at least one dictionary 
  //

#ifdef DICTSTACK_CACHE
    clear_dict_from_cache(*(d.begin()));
    (*(d.begin()))->remove_dictstack_reference();
#endif
    d.pop_front();
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

#ifdef DICTSTACK_CACHE
    dict->add_dictstack_reference();
    // This call will remove all names in the dict from the name cache.
    clear_dict_from_cache(dict);
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

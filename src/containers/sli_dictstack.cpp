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
#define DICTSTACK_CACHE
namespace sli3
{

DictionaryStack::DictionaryStack( )
{
}

DictionaryStack::DictionaryStack(const DictionaryStack &ds )
  :  d(ds.d),
     base_(ds.base_),
     cache_(ds.cache_),
     basecache_(ds.basecache_)
{
  // Each held Dictionary now has one extra owner (this copy of the
  // stack). Bump the refcount to match; pop()/clear()/dtor each
  // pair their decrement with the push add_reference.
  for (Dictionary *dict : d)
    dict->add_reference();
}

DictionaryStack::~DictionaryStack()
{
    // Release our cached snapshot before tearing the stack down so
    // the array's destructor (which decrements each dict-entry's
    // refcount) runs while the dicts are still alive.
    invalidate_snapshot();

    // Each push() did add_reference() to keep the dict alive while
    // it was on the stack. The destructor must pair every push with
    // a remove_reference; otherwise dictionaries left on the stack
    // at teardown leak forever.
    for (std::list<Dictionary *>::iterator i = d.begin();
         i != d.end(); ++i)
    {
      (*i)->clear();
      (*i)->remove_reference();
    }
}

void DictionaryStack::invalidate_snapshot()
{
    if (snapshot_ != nullptr)
    {
        // Drop our reference. If outside holders (a /d binding,
        // a Token on the operand stack) keep it alive, the array
        // survives with a smaller refcount; if we were the last,
        // remove_reference deletes it.
        snapshot_->remove_reference();
        snapshot_ = nullptr;
    }
}

TokenArray *DictionaryStack::snapshot(SLIInterpreter &sli)
{
    if (snapshot_ == nullptr)
    {
        // First call since the last mutation: materialise. The fresh
        // TokenArray has refs_=1 — exactly the ref we keep in the
        // cache slot. add_reference for each dict happens inside
        // toArray.
        snapshot_ = new TokenArray();
        toArray(sli, *snapshot_);
    }
    // Hand out one ref to the caller (matches the implicit +1 that
    // `new TokenArray()` used to give the old DictstackFunction
    // body). The Token-construct / push / local-destruct dance at
    // the call site will net-leave that +1 on the operand stack.
    snapshot_->add_reference();
    return snapshot_;
}

void DictionaryStack::undef(Name const & n)
{
    // Erase from the TOP dictionary only -- PostScript semantics.
    if (d.empty())
      throw UndefinedName(n.toString());

    size_t num_erased = (*d.begin())->erase(n);

    if (num_erased == 0)
      throw UndefinedName(n.toString());

    // Cache invalidation MUST be unconditional, because cache_token()
    // populates unconditionally (cache_/basecache_ store raw Token*
    // pointers into dict map nodes; erasing the entry leaves a
    // dangling pointer).
    clear_token_from_cache(n);
    clear_token_from_basecache(n);
}

void DictionaryStack::basedef( Name const & n, const Token &t)
{
  //
  // insert (n,t) in bottom level dictionary
  // dictionary stack must contain at least one dictionary
  // VoidToken is an illegal value for t.
  if (base_ == nullptr)
    throw UndefinedName("base dictionary not set");
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

  // dstack identity changes — snapshot is stale.
  invalidate_snapshot();

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
    // dstack identity changes — snapshot is stale.
    invalidate_snapshot();

    // Mirror destructor refcount accounting: each entry on the stack
    // owes one remove_reference (paired with push's add_reference).
    for (std::list<Dictionary *>::iterator i = d.begin();
         i != d.end(); ++i)
    {
      (*i)->remove_reference();
    }
    d.clear();
    base_ = nullptr;
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
  // Bottom-first snapshot: arr[0] = bottom dict, arr[size-1] = top.
  // Inverse of restore_from. Reserve up front so push_back never
  // reallocates — on the dictstack save/restore hot path that's a
  // measurable win (the array is freshly allocated per dictstack
  // call, so growth would mean every call goes through a
  // 1→2→4-element realloc chain).
  //
  ta.clear();
  ta.reserve(d.size());

  SLIType *dict_type = sli.get_type(sli3::dictionarytype);
  for (auto it = d.rbegin(); it != d.rend(); ++it)
  {
    Token dicttoken;
    dicttoken.type_ = dict_type;
    dicttoken.data_.dict_val = *it;
    dicttoken.add_reference();  // raw-assigned payload: bump dict refcount so
                                // the local destructor's remove_reference is
                                // balanced (otherwise vector entry's bump and
                                // local's drop cancel, leaving the array
                                // entry without a true reference).
    ta.push_back(dicttoken);
  }
}

void DictionaryStack::push(Token const & dicttoken)
{
  assert(dicttoken.is_of_type(sli3::dictionarytype));
  Dictionary *dict=dicttoken.data_.dict_val;

  // dstack identity changes — snapshot is stale.
  invalidate_snapshot();
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

void DictionaryStack::restore_from(TokenArray const &ta)
{
    const size_t n = ta.size();

    // Fast path: same dicts, same order. The dictstack list keeps top
    // at front (push_front), so d.rbegin() walks bottom→top, which
    // matches ta[0]..ta[n-1]. Identity match means refcounts and
    // cache_/basecache_ entries are still valid — skip the rebuild.
    if (n == d.size())
    {
        bool match = true;
        size_t k = 0;
        for (auto it = d.rbegin(); it != d.rend(); ++it, ++k)
        {
            if (*it != ta[k].data_.dict_val)
            {
                match = false;
                break;
            }
        }
        if (match)
            return;
    }

    // Slow path: real rebuild. clear() drops every dict's refcount
    // and wipes both caches; the subsequent pushes don't need to
    // walk each dict's keys again (the cache is already all zero),
    // so we inline a no-invalidate push instead of calling push().
    clear();
    for (size_t k = 0; k < n; ++k)
    {
        Dictionary *dict = ta[k].data_.dict_val;
        assert(dict != nullptr);
        dict->add_reference();
#ifdef DICTSTACK_CACHE
        dict->add_dictstack_reference();
#endif
        d.push_front(dict);
    }
    set_basedict();
}

void DictionaryStack::set_basedict()
{
    assert(!d.empty());
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
    // Drop refs we held on the old contents.
    for (Dictionary *dict : d)
      dict->remove_reference();

    d         = ds.d;
    base_     = ds.base_;
    cache_    = ds.cache_;
    basecache_= ds.basecache_;

    // Bump refs for the new contents.
    for (Dictionary *dict : d)
      dict->add_reference();
  }
  return *this;
}
}

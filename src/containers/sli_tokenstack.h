/*
 *  tokenstack.h
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

#ifndef SLI_TOKENSTACK_H
#define SLI_TOKENSTACK_H
/* 
    SLI's stack for tokens
*/

#include "sli_token.h"
#include "sli_array.h"
#include <cassert>

/* This stack implementation assumes that functions are only called,
   if the necessary pre-requisites are fulfilled. The code will break
   otherwise.
*/
namespace sli3
{
class TokenStack : private TokenArray
{
public:
    TokenStack(size_t n ) 
            : TokenArray(0,Token(),n) {}
    TokenStack(const TokenArray& ta) 
            : TokenArray(ta) {}

  using TokenArray::reserve;
  using TokenArray::reserve_token;

  void clear(void)
  {
    erase(begin(),end());
  }
  
  void push(const Token& e)
  {
    push_back(e);
  }
  
  void pop(void)
  {
    assert(load() > 0);
    pop_back();
  }

  void pop(size_t n)
  {
    assert(load() >= n);
    erase(end()-n, end());
  }


  Token& top(void)
    {
      assert(load() > 0);
      return *(end()-1);
    }

  const Token& top(void) const
  {
    assert(load() > 0);
    return *(end()-1);
  }

  const Token& pick(size_t i) const
  {
    assert(i < load());
    return *(end()-i-1);
  }

  Token& pick(size_t i)
    {
      assert(i < load());
      return *(end()-i-1);
    }

  using TokenArray::empty;

  void swap(void)
  {
    assert(load() >= 2);
    (end()-1)->swap(*(end()-2));
  }

  void swap(Token &e)
  {
    assert(load() > 0);
    (end()-1)->swap(e);
  }

  void index(size_t i)
  {
    push(pick(i));
  }

  void roll(size_t n, long k)
  {
    // n == 0: nothing to rotate. The previous code did `k % n`,
    // which divides by zero. PostScript-style semantics: rolling
    // zero elements is a no-op regardless of k.
    if (n == 0) return;

    if (k>=0)
      {
	rotate(end()-n,end()-(k%n),end());
      }
    else
      {
	rotate(end()-n,end()-(n+k)%n,end());
      }
  }
            
  // ----------------------------------------------------------------
  // size() vs load() — read carefully.
  //
  // size() returns the underlying storage CAPACITY, NOT the number of
  // pushed elements. This is the NEST 2.x convention (verified against
  // nest-simulator v2.20.2 sli/tokenstack.h: `size() { return
  // TokenArrayObj::capacity(); }`). It is preserved here for API
  // compatibility — sli3 is meant to run NEST 2.x SLI scripts whose
  // C++ surface depends on this behaviour.
  //
  // To get the number of elements currently on the stack, use load().
  //
  // If you find yourself reaching for size() expecting "stack depth",
  // you almost certainly want load(). The single legitimate use of
  // size() is debugging / diagnostics where the buffer reservation
  // matters.
  // ----------------------------------------------------------------
  size_t size(void) const        {return TokenArray::capacity();}
  size_t load(void) const        {return TokenArray::size();}

  void dump(std::ostream &) const;
  
  TokenArray * toArray(void) const
  {
    return new TokenArray(*this);
  }
    
};
}
#endif

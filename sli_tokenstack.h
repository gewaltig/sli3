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
        pop_back();
    }
 
    
    void pop(size_t n)
    {
        erase(end()-n, end());
    }
 
    
    Token& top(void)
    {
        return *(end()-1);
    }

    const Token& top(void) const
    {
        return *(end()-1);
    }

    const Token& pick(size_t i) const
    {
        return *(end()-i-1);           
    }
  
    Token& pick(size_t i)
    {
        return *(end()-i-1);
    }

    using TokenArray::empty;
        
    void swap(void)
    {
      (end()-1)->swap(*(end()-2));
    }

    void swap(Token &e)
    {
      (end()-1)->swap(e);
    }

    void index(size_t i)
    {
      push(pick(i));
    }

    void roll(size_t n, long k)
    {
      if (k>=0)
      {
	rotate(end()-n,end()-(k%n),end());
      }
      else 
      {
	rotate(end()-n,end()-(n+k)%n,end());
      }
    }            
            
    size_t size(void) const        {return TokenArray::capacity();}
    size_t load(void) const        {return TokenArray::size();} 

    void dump(std::ostream &) const;

    TokenArray toArray(void) const
    {
      return TokenArray(*this);
    }
    
};
}
#endif

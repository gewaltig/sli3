/*
 *  tarrayobj.h
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

#ifndef TOKENARRAY_H
#define TOKENARRAY_H
/* 
    Array of Token
*/

#include <typeinfo>
#include <cstddef>
#include "sli_token.h"

namespace sli3
{
#define ARRAY_ALLOC_SIZE 64

class Token;

class TokenArray
{
 private:
  Token* p;
  Token* begin_of_free_storage;
  Token* end_of_free_storage;
  unsigned int alloc_block_size;
  unsigned int refs_;
  
//  bool homogeneous;

  void allocate(size_t, size_t, size_t, const Token & = Token());
    
  void copy_right_to_left(Token *to, Token *from, Token *end)
  {
    while (from>end)
      {
	*to = *from;
	--from;
	--to;
      }
  }

  void copy_left_to_right(Token *to, Token *from, Token *end)
  {
    while (from<end)
      {
	*to = *from;
	++from;
	++to;
      }
  }

 public:
  static size_t allocations;
    
    TokenArray(void)
            :p(NULL),begin_of_free_storage(NULL),
             end_of_free_storage(NULL),
             alloc_block_size(ARRAY_ALLOC_SIZE), 
	     refs_(1)
    {
      ++allocations;
    }
    
    TokenArray(size_t , const Token & = Token(), size_t =  0);
    TokenArray(const TokenArray&);

    virtual ~TokenArray();

    /**
     * This function returns a pointer to a copy of the token array with
     * just one reference. 
     */ 
    TokenArray *detach()
    {
      if( refs_ ==1)
	return this;

      --refs_;
      return new TokenArray(*this);
    }

    Token * begin() const
    {
        return p;
    }
    
    Token * end() const
    {
        return begin_of_free_storage;
    }

    size_t size(void) const
    {
        return (size_t)(begin_of_free_storage-p);
    }
        
    size_t capacity(void) const
    {
        return (size_t)(end_of_free_storage - p);
    }
    
    Token & operator[](size_t i) { return p[i]; }
    
    const Token & operator[](size_t i) const
    { return p[i]; }

    const Token & get(long i) const
    {
      return *(p+i);
      //      return p[i];
    }

    bool index_is_valid(size_t i) const
    {
      return ((p+i) < begin_of_free_storage);
    }

    void rotate(Token *, Token *, Token *);


    // Memory allocation
    
    bool shrink(void);
    bool reserve(size_t);
    
    unsigned int references(void)
    {
      return refs_;
    }

    unsigned int remove_reference()
    {
      --refs_;
      if(refs_==0)
	{
	  delete this;
	  return 0;
	}
      
      return refs_;
    }

    unsigned int add_reference()
    {
      return ++refs_;
    }

    void resize(size_t, size_t, const Token & = Token());
    void resize(size_t, const Token & = Token());

    void reserve_token(int n)
    {
      if((end_of_free_storage-begin_of_free_storage) < n)
      	reserve(size()+n);
    }
        // Insertion, deletion
    void push_back(const Token &t)
    {
      reserve_token(1);
      (begin_of_free_storage++)->init(t);
    }

    void pop_back(void)
    {
      (--begin_of_free_storage)->clear();
    }

  // Erase the range given by the iterators.
    void erase(size_t , size_t);   
    void erase(Token *, Token *);
    void erase(Token *tp)
    {
        erase(tp,tp+1);
    }

  // Reduce the array to the range given by the iterators
    void reduce(Token *, Token *);
    void reduce(size_t, size_t);

    /**
       Insert n tokens, starting at position i.
     */
    void insert(size_t i, size_t n= 1, const Token& = Token());
    void insert(size_t i, const Token& t)
    {
        insert(i,1,t);
    }
    void insert(size_t, TokenArray const &);
    

    /**
     * Assign sub-array.
     */ 
    void assign(const TokenArray &, size_t, size_t);

    void append(TokenArray const&);

    /**
     * Erade contents of array and free memory.
     */
    void clear(void);
        
        
    const TokenArray & operator=(const TokenArray &);
    
    bool operator==(const TokenArray &) const;

    bool empty(void) const
    {
        return size()==0;
    }
    
    void info(std::ostream &) const;

  static size_t getallocations(void)
    { return allocations;}

  bool valid(void) const; // check integrity
    
};

std::ostream & operator<<(std::ostream& , const TokenArray&);

}

#endif



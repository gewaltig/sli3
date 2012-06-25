/*
 *  tarrayobj.cc
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

#include "sli_array.h"
#include "sli_token.h"

#include <cassert>
namespace sli3
{

size_t TokenArray::allocations=0;

    TokenArray::TokenArray(size_t s, const Token &t, size_t alloc)
        :p(NULL),begin_of_free_storage(NULL),
         end_of_free_storage(NULL),alloc_block_size(ARRAY_ALLOC_SIZE),refs_(1)
    {
	size_t a = (alloc == 0)? s : alloc;
	
	resize(s,a,t);
    }


    TokenArray::TokenArray(const TokenArray &a)
        :p(NULL),begin_of_free_storage(NULL),
         end_of_free_storage(NULL),alloc_block_size(ARRAY_ALLOC_SIZE),refs_(1)
    {
	if(a.p != NULL)
	{
	    resize(a.size(),a.alloc_block_size,Token());
	    Token *from = a.p;
	    Token *to   = p;
	    
	    while(to < begin_of_free_storage)
		*to++ = *from++;
	}
	++allocations;
    }

    
TokenArray::~TokenArray()
{
    if(p)
        delete [] p;
}

void TokenArray::allocate(size_t new_s, size_t new_c, size_t new_a, const Token &t )
{
// This resize function is private and does an unconditional resize, using
// all supplied parameters.

    alloc_block_size = new_a;
    
    size_t min_l;
    size_t old_s = size();

    assert(new_c != 0);
    assert(new_a != 0);
    
    Token *h = new Token[new_c];
    assert(h != NULL);

    if(t != Token())
      for(Token *hi=h; hi <h+new_c; ++hi)
	(*hi)=t;

    end_of_free_storage  = h + new_c;  // [,) convention
    begin_of_free_storage= h + new_s;
    
    if(p!=NULL)
    {
        if(old_s < new_s)
        {
            min_l = old_s;
        }
        else
        {
            min_l = new_s;
        }
            
        for(size_t i=0; i< min_l; ++i) // copy old parts
            h[i].move(p[i]);
        delete [] p;
    }
    p = h;
    assert(p!=NULL);

    ++allocations;
}

void TokenArray::resize(size_t s, size_t alloc, const Token &t )
{
    alloc_block_size = (alloc == 0)? alloc_block_size : alloc;

    if((s!=size() && (s!=0)) || (size()==0 && alloc_block_size != 0))
        allocate(s, s+alloc_block_size, alloc_block_size, t);
}

void TokenArray::resize(size_t s,  const Token &t )
{
    resize(s,alloc_block_size,t);
}

const TokenArray & TokenArray::operator=(const TokenArray &a)
{
    if(capacity() >= a.size())
            // This branch also covers the case where a is the null-vector.
    {
        Token *to = begin();
        Token *from = a.begin();
        while(from < a.end())
            *to++ = *from++;
        
        while(to < end())
        {
            to->clear();
            to++;
        }
        begin_of_free_storage = p+a.size();
        
        assert(begin_of_free_storage <= end_of_free_storage);

    } else
    {
        
        if(p !=NULL)
        {
            delete [] p;
            p = NULL;
        }
    
        resize(a.size(),a.alloc_block_size);
        Token *to = begin();
        Token *from = a.begin();
        while(from < a.end())
            *to++ = *from++;
        begin_of_free_storage = to;
        assert(begin_of_free_storage <= end_of_free_storage);
    }
    
    return *this;
}


bool TokenArray::shrink(void)
{
    size_t new_capacity = size();

    if( new_capacity < capacity())
    {
      allocate(size(), new_capacity, alloc_block_size);
      return true;
    }
    return false;
}

bool TokenArray::reserve(size_t new_capacity)
{
    if(new_capacity > capacity() )
    {
      allocate(size(), new_capacity, alloc_block_size);        
      return true;
    }
    return false;
}


void TokenArray::rotate(Token *first, Token *middle, Token *last)
{

// This algorithm is taken from the HP STL implementation.
    if((first < middle) && (middle < last)) 
        for(Token *i = middle; ;)
        {
            
            first->swap(*i);
            i++; first++;
            
            if(first == middle)
            {
                if(i == last) return;
                middle = i;
            }
            else if (i == last)
                i = middle;
        }
}

void TokenArray::erase(Token* first, Token *last)
{
   // array is decreasing. we move elements after point of
   // erasure from right to left
   Token *from = last; 
   Token *to   = first;
   Token *end = begin_of_free_storage; // 1 ahead  as conventional

   while (from<end)
   {
       to->move(*from);
       ++from;
       ++to;
   } 
   
   while (last>to)      // if sequence we have to erase is 
   {                    // longer than the sequence to the
       --last;            // right of it, we explicitly delete the 
       last->clear();
   }                    
   
   begin_of_free_storage=to;
}

// as for strings erase tolerates i+n >=  size()
//
void TokenArray::erase(size_t i, size_t n)
{
    if (i+n<size())
	erase(p+i, p+i+n );
    else
	erase(p+(i), p+size());
}

void TokenArray::clear(void)
{
  for(Token *t=p; t<end_of_free_storage; ++t)
    {
      if(t)
	t->clear();
    }
  if(p)
    delete [] p;
  p = begin_of_free_storage = end_of_free_storage = NULL;
  alloc_block_size = 1;
}

// reduce() could be further optimized by testing wether the
// new size leads to a resize. In this case, one could simply
// re-construct the array with the sub-array.

void TokenArray::reduce(Token* first, Token *last)
{
    assert(last<=end());
    assert(first>= p);

        // First step: shift all elements to the begin of
        // the array.
    Token *i= p, *l= first;

    if(first > begin())
    {
        while(l < last)
        {
            i->move(*l);
            i++; l++;
        }
        assert( l == last);
    }
    else
        i = last;

    assert( i == p + (last - first));

    while (i < end()) 
    {
        i->clear();
        i++;
    }
    begin_of_free_storage = p + (size_t)(last-first);
    //shrink();
}

// as assign for strings reduce tolerates i+n >= size() 
//
void TokenArray::reduce(size_t i, size_t n)
{ 
 if (i+n<size())
  reduce(p+i, p+i+n );
 else
  reduce(p+(i), p+size());
}

void TokenArray::insert(size_t i, size_t n, const Token &t)
{
// pointer argument pos would not be efficient because we
// have to recompute pointer anyway after reallocation 
 
    reserve(size()+n);                                        // reallocate if necessary
    
    Token *pos  = p + i;                     // pointer to element i (starting with 0)
    Token *from = begin_of_free_storage-1;   // first Token which has to be moved
    Token *to   = from + n;                  // new location of first Token
        
    while(from >= pos)                     
    {
	to->move(*from);         // move             
	--from; --to;            // NULL before
    }
        
    for(size_t i=0; i<n; ++i)                // insert n copies of Token t;
	*(pos++) = t;
    
    begin_of_free_storage+=n;                // new size is old + n
}

void TokenArray::insert(size_t i, TokenArray const& a)
{
    reserve(size()+a.size());                                 // reallocate if necessary
    assert(begin_of_free_storage + a.size() <= end_of_free_storage);  // check 

    Token *pos  = p + i;                     // pointer to element i (starting with 0)
    Token *from = begin_of_free_storage-1;   // first Token which has to be moved
    Token *to   = from + a.size();           // new location of first Token
        

    
    while(from >= pos)                     
    {
	to->move(*from);
	--from; --to;     
    }

    from = a.p;
    to   = p + i;
    
    while( from < a.end())                
    {                                     
        to->move(*from);
        ++from;
        ++to;
    }

    begin_of_free_storage+=a.size();         // new size is old + n
}


void TokenArray::assign(const TokenArray &a, size_t i, size_t n)
{
  reserve(n);

  Token *from = a.begin()+i;
  Token *end  = a.begin()+i+n;  
  Token *to   = p;          
  
  while (from < end)       
  {   
    *to = *from;
    ++from;
    ++to;
  }

  begin_of_free_storage = p+n; 
}


void TokenArray::append(TokenArray const &a ) 
{
    reserve(size()+a.size());                                 // reallocate if necessary
    assert(begin_of_free_storage + a.size() <= end_of_free_storage);  // check 

    Token *from = a.p;
    Token *to = begin_of_free_storage;

    while( from < a.end())        
    {                          
	*to = *from;           
        ++from;
        ++to;
    }

    begin_of_free_storage+=a.size();         // new size is old + n
}


bool TokenArray::operator==(const TokenArray &a) const
{

  //std::cout << "comparison of TokenArray" << std::endl;
  //std::cout << "p:   " << p << std::endl;
  //std::cout << "a.p: " << a.p << std::endl;

    if( p == a.p )
        return true;

    // experimentally replaced by line below 090120, Diesmann
    // because [] cvx has non NULL p
    //
    //    if( p == NULL || a.p == NULL || size() != a.size())
    //    return false;

    if( size() != a.size() )
        return false;

    Token *i= begin(),*j = a.begin();
    while(i < end())
        if(!(*i++ == *j++))
            return false;
    return true;
}

void TokenArray::info(std::ostream& out) const
{
    out << "TokenArray::info\n";
    out << "p    = " << p << std::endl;
    out << "bofs = " << begin_of_free_storage << std::endl;
    out << "eofs = " << end_of_free_storage << std::endl;
    out << "abs  = " << alloc_block_size << std::endl;
}

bool TokenArray::valid(void) const
{
  if(p == NULL)
  {
    std::cerr << "TokenArray::valid: Data pointer missing!" << std::endl;
    return false;
  }

  if(begin_of_free_storage == NULL)
  {
    std::cerr << "TokenArray::valid: begin of free storage pointer missing!"
	 << std::endl;
    return false;
  }

  if(end_of_free_storage == NULL)
  {
    std::cerr << "TokenArray::valid: end of free storage pointer missing!"
	 << std::endl;
    return false;
  }

  if(begin_of_free_storage >end_of_free_storage)
  {
    std::cerr << "TokenArray::valid: begin_of_free_storage  > end_of_free_storage !"
	 << std::endl;
    return false;
  }
    
  return true;
}
  

std::ostream & operator<<(std::ostream& out, const TokenArray& a)
{
    
    for(Token *i = a.begin(); i< a.end() ; ++i)
        out << *i << ' ';

    return out;
    
}

} // end o namespace

/*
 *  allocator.cpp
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

#include "sli_allocator.h"

sli3::pool::pool()
  : initial_block_size(1024),
    growth_factor(1),
    block_size(initial_block_size),
    el_size(sizeof(link)),
    instantiations(0),
    total(0),
    capacity(0),
    chunks(0), 
    head(0),
    initialized_(false)
{}

sli3::pool::pool(const sli3::pool &p)
  : initial_block_size(p.initial_block_size),
    growth_factor(p.growth_factor),
    block_size(initial_block_size),
    el_size(sizeof(link)),
    instantiations(0),
    total(0),
    capacity(0),
    chunks(0), 
    head(0),
    initialized_(false)
{}


sli3::pool::pool(size_t n, size_t initial, size_t growth)
  : initial_block_size(initial),
    growth_factor(growth),
    block_size(initial_block_size),
    el_size( ( n < sizeof(link))? sizeof(link): n),
    instantiations(0),
    total(0),
    capacity(0),
    chunks(0), 
    head(0),
    initialized_(true)
{}

void sli3::pool::init(size_t n, size_t initial, size_t growth)
{
  assert(instantiations == 0);

  initialized_=true;

  initial_block_size=initial;
  growth_factor=growth;
  block_size=initial_block_size;
  el_size =  ( n < sizeof(link))? sizeof(link): n;
  instantiations=0;
  total=0;
  capacity=0;
  chunks=0; 
  head=0;
}

sli3::pool::~pool()
{
  chunk *n=chunks;
  while(n)
  {
    chunk *p=n;
    n=n->next;
    delete p;
  }
}

sli3::pool & sli3::pool::operator=(const sli3::pool&p)
{
  if(&p == this)
    return *this;

  initial_block_size=p.initial_block_size;
  growth_factor=p.growth_factor;
  block_size=initial_block_size;
  el_size=p.el_size;
  instantiations=0;
  total=0;
  chunks=0; 
  head=0;
  initialized_=false;
  
  return *this;
}

void sli3::pool::grow(size_t nelements)
{
  chunk *n = new chunk(nelements*el_size);
  total    += nelements;

  n->next = chunks;
  chunks = n;
  char *start= n->mem;
  char *last= &start[ (nelements-1)*el_size];

  for(char *p=start; p<last; p+=el_size)
    reinterpret_cast<link*>(p)->next=reinterpret_cast<link*>(p+el_size);
  reinterpret_cast<link*>(last)->next = NULL;
  head = reinterpret_cast<link*>(start);

}

void sli3::pool::grow(void)
{
  grow(block_size);
  block_size *= growth_factor;
}

void sli3::pool::reserve(size_t n)
{
    const size_t capacity=total-instantiations; 
    if(capacity < n)
    grow(((n-capacity)/block_size+1)*block_size);
}

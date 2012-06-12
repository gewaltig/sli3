/*
 *  aggregatedatum.h
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

#ifndef AGGREGATEDATUM_H
#define AGGREGATEDATUM_H
#include "sli_token.h"
#include "sli_allocator.h"
#include "sli_config.h"

namespace sli3
{
    class DatumConverter;

/* 
    Datum template for aggregate data types.
*/

/************************************************
 The AggregateDatum template should be used for all
 Datum objects which contain class objects (i.e. no
 trivial types like int, long, char, etc.)

 AggregateDatum inherits some virtual functions from
 its base class Datum which must be supplied.
 Usually destruction should be trivial, though a
 virtual destructor must be supplied.

 In order to avoid ambiguities with potential
 base classes, no virtual operators should be used
 in the Datum class, rather "unique" virtual
 function names should be used.

 Particularly, the operator<< should not be defined
 for base class Datum.
*************************************************/

    template <class C>
    class AggregateToken : public C
    {
    private:
	virtual Token * clone(SLIIType &t) const
	    {
		return C::clone(t, *this);
	    }
	
    public:
	AggregateDatum();
	AggregateDatum(const AggregateDatum<Ct> &d);
	AggregateDatum(const C& c); 
	
	virtual ~AggregateDatum() {}
	
	virtual void print(std::ostream &) const;
	virtual void pprint(std::ostream &) const;
	virtual void list(std::ostream &, std::string, int) const;
	virtual void input_form(std::ostream &) const;
	virtual void info(std::ostream &) const;
  

	bool equals(const Datum *dat) const
	    {
		// The following construct works around the problem, that
		// a direct dynamic_cast<const GenericDatum<D> * > does not seem
		// to work.
		
		const AggregateDatum<C,slt>
		    *ddc=dynamic_cast<AggregateDatum<C, slt> * >(const_cast< Datum *>(dat));
		
		if(ddc == NULL)
		    return false;
		
		return  static_cast< C >(*ddc) == static_cast<C>(*this);
		
	    }
	
	static void * operator new(size_t size)
	    {
		if(size != memory.size_of())
		    return ::operator new(size);
		return memory.alloc();
	    }
	
	static void operator delete(void *p, size_t size)
	    {
		if(p == NULL)
		    return;
		if(size != memory.size_of())
		{
		    ::operator delete(p);
		    return;
		}
		memory.free(p);
	    }
	
	/**
	 * Accept a DatumConverter as a visitor to the datum (visitor pattern).
	 * This member has to be overridden in the derived classes
	 * to call visit and passing themselves as an argument.
	 */
	void use_converter(DatumConverter &);
};



#endif



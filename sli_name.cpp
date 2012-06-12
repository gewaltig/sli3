/*
 *  sli_name.cc
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

#include "sli_name.h"

#include <iostream>
#include <iomanip>


std::size_t sli3::Name::capacity()
{
    return  sli3::Name::handleTableInstance_().size();
}

std::size_t sli3::Name::num_handles()
{
    return sli3::Name::handleTableInstance_().size();
}


void sli3::Name::list_handles(std::ostream& out)
{
    HandleTable_ &table=sli3::Name::handleTableInstance_();
    std::size_t num_handles = table.size();
    
    out << "Handle Table: \n";
    out << "Total number of names : "<< num_handles << std::endl;
    
  for ( std::size_t n=0; n< num_handles; ++n)
  {
      out << std::setw(6) << n << ": "
	  << table[n] << std::endl;
  }
}

void sli3::Name::print_handle(std::ostream &o) const
{
    o << "/"<< handleTableInstance_()[handle_] << '('<<handle_ << ')';
}

// ---------------------------------------------------------------


const std::string& sli3::Name::toString(void) const
{
    return handleTableInstance_()[handle_];
}

unsigned int sli3::Name::insert(const std::string &s)
{
    sli3::Name::HandleMap_ &map=sli3::Name::handleMapInstance_();
    sli3::Name::HandleMap_::const_iterator where=map.find(s);

    if( where == map.end() )
    {
	// The following is more comlex code than a simple
	// handleMap_[s] = Handle(s), but it avoids the creation
	// of spurious Handle objects. HEP 2007-05-24
	HandleTable_ &table=sli3::Name::handleTableInstance_();
	unsigned int newhandle= table.size();
	map.insert(std::make_pair(s, newhandle));
	table.push_back(s);
	return newhandle;
    }
    else
	return ((*where).second);
}

void sli3::Name::list(std::ostream &out)
{
    sli3::Name::HandleMap_ &map=handleMapInstance_();
    out << "\nHandle Map content:" << std::endl;
    for ( sli3::Name::HandleMap_::const_iterator where = map.begin(); 
	  where != map.end(); ++where )
    {
	out << (*where).first << " -> "
	    << (*where).second
	    << std::endl;
    }
    
    out << "\nHandle::handleTable_ content" << std::endl;
    sli3::Name::list_handles(out);
}

void sli3::Name::info(std::ostream &out)
{
    sli3::Name::list_handles(out);
}


std::ostream & sli3::operator<< (std::ostream & o, const sli3::Name &n)
{
    o << n.toString().c_str() ;
    return o;
}


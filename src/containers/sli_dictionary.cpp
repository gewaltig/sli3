/*
 *  dict.cc
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

#include "sli_dictionary.h"
#include "sli_exceptions.h"
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <string>

namespace sli3
{

Dictionary::~Dictionary()
{
    clear();
}

const DictToken& Dictionary::operator[](const char *n) const
{
  return operator[](Name(n));
}

DictToken& Dictionary::operator[](const char *n)
{
  return operator[](Name(n));
}


void Dictionary::clear()
{
    // now clear dictionary itself; HEP 2004-09-08
    TokenMap::clear();
}

void Dictionary::info(std::ostream &out) const
{
  out.setf(std::ios::left);
  if(size()>0)
  {
    // copy to vector and sort
    typedef std::vector<std::pair<Name, DictToken> >DataVec;
    DataVec data;
    std::copy(begin(), end(), std::inserter(data, data.begin()));
    std::sort(data.begin(), data.end(), DictItemLexicalOrder());

    out << "--------------------------------------------------" << std::endl;
    out << std::setw(25) <<  "Name" << std::setw(20) << "Type" <<  "Value" << std::endl;
    out << "--------------------------------------------------" << std::endl;

    // Iterate the sorted vector. The previous code iterated `*this`
    // directly, which yielded the std::map's natural Name-handle
    // (i.e. interning) order rather than the lexicographic order
    // the sort had just established.
    for(DataVec::const_iterator where = data.begin() ;
	where != data.end() ; ++ where)
    {
      out  << std::setw(25) << where->first
	   << std::setw(20) << where->second.get_typename()
	   << where->second
	   << std::endl;
    }
    out << "--------------------------------------------------" << std::endl;
  }
  out << "Total number of entries: "<< size() << std::endl;

  out.unsetf(std::ios::left);

}

// add_dict / remove_dict removed in Stage 2.8 -- declared in the
// header for years, bodies commented out, no callers.

void Dictionary::remove(Name const & n)
{
  TokenMap::iterator it = find(n);
  if ( it != end() )
    erase(it);
}

void Dictionary::clear_access_flags()
{
  for ( TokenMap::iterator it = TokenMap::begin() ;
	it != TokenMap::end() ; ++it )
  {
    // Clear the flag on this entry. Previously the loop body only
    // recursed into nested dicts and never reset its own entries'
    // access flags -- so all_accessed() returned a stale answer
    // for any non-dict entry that had been read before.
    it->second.clear_access_flag();

    if ( it->second.is_of_type(sli3::dictionarytype)
         && it->second.data_.dict_val != nullptr )
    {
      it->second.data_.dict_val->clear_access_flags();
    }
  }
}

bool Dictionary::all_accessed_(std::string& missed, std::string prefix) const
{
  missed = "";

  // build list of all non-accessed Token names
  for ( TokenMap::const_iterator it = TokenMap::begin() ;
	it != TokenMap::end() ; ++it )
  {
    if ( !it->second.accessed() )
	missed = missed + " " + prefix + it->first.toString();
    else if ( it->second.is_of_type(sli3::dictionarytype) )
    {
      // recursively check if nested dictionary content was accessed
      // see also comments in clear_access_flags()

      // this sets access flag on it->second, but that does not matter,
      // since it is anyways set, otherwise we would not be recursing
      Dictionary* subdict = it->second.data_.dict_val;

      subdict->all_accessed_(missed, prefix + it->first.toString() + "::");
    }
  }

  return missed.empty();
}


std::ostream & operator<<(std::ostream &out, const Dictionary &d)
{
    out << "<< ";

    for(TokenMap::const_iterator where = d.begin(); where != d.end(); where ++)
    {
        out << '/' << (*where).first << ' '
	    << (*where).second << ' ';
    }
    out << ">>";

    return out;

}

bool Dictionary::DictItemLexicalOrder::nocase_compare(char c1, char c2)
{
  return std::toupper(c1) < std::toupper(c2);
}

}

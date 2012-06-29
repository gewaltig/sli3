#ifndef SLI_NAME_H
#define SLI_NAME_H
/*
 *  name.h
 *
 *  This file is part of NEST
 *
 *  Copyright (C) 2004-2007 by
 *  The NEST Initiative
 *
 *  See the file AUTHORS for details.
 *
 *  Permission is granted to compile and modify
 *  this file for non-commercial use.
 *  See the file LICENSE for details.
 *
 */

#include <cassert>
#include <map>
#include <string>
#include <deque>
#include <iostream>

/**
 * Represent strings by ints to facilitate fast comparison.
 *
 * Each Name object represents a string by a unique integer number.
 * Comparing Name objects instead of comparing strings directly,
 * reduces the complexity of string comparison to that of int comparison.
 *
 * Each Name object contains a Handle to the string it represents. Strings are 
 * mapped to Handles via an associative array. Handles are stored in a table,
 * and each Handle contains its own index into this table as unique ID, as
 * well as the string represented. Fast comparison of Name objects is achieved
 * by comparing the indices stored in the handles. Reference counting 
 * permits deletion of unused Handles.
 *
 * @note Any string read by the interpreter should be converted to a Name
 * at once.
 * @note class Name maintains two static lookup tables and is thus not
 * thread-safe.
 *
 */
namespace sli3
{
  class Name
  {
      
  public:
      typedef unsigned int handle_t;
      /**
       * Create Name without value.
       */
      Name()                  : handle_(0) {}
      Name(long h)            : handle_(h)
	  {}
      
      Name(const char s[])       : handle_(insert(std::string(s))) {} 
      Name(const std::string &s) : handle_(insert(s)) {}
      Name(const Name &n)        : handle_(n.handle_) {}
      operator int() const
      {
	return handle_;
      }
      
      /**
       * Return string represented by Name.
       */
      const std::string& toString(void) const;
      
      /**
       * Return table index for Name object.
       */
      handle_t toIndex(void)  const 
	  {
	      return handle_;
	  }
      
      bool operator== (const Name &n) const
	  {
	      return handle_ == n.handle_;
	  }
      
      bool operator!= (const Name &n) const
	  {
	      return !(handle_ == n.handle_);
	  }
      
      /**
       * Non-alphabetic ordering of names.
       * Entering Name's into dictionaries requires ordering. Ordering based
       * on string comparison would be very slow. We thus compare based on
       * table indices.
       */
      bool operator< (const Name &n) const
	  {
	      return handle_ < n.handle_;
	  }
      
      static bool lookup(const std::string &s)
	  {
	      HandleMap_ &table=handleMapInstance_();
	      return (table.find(s) != table.end());
	  }
      
      static
      size_t capacity();
      static 
      size_t num_handles();
      
      void print_handle(std::ostream&) const;
      
      static void list_handles(std::ostream &);
      static void list(std::ostream &);
      static void info(std::ostream &);
      
  private:
      handle_t insert(const std::string&);  
      
      /**
       * Datatype for map from strings to handles. 
       */
      typedef std::map<std::string, handle_t> HandleMap_;
      typedef std::deque<std::string> HandleTable_;
      
      /** 
       * Function returning a reference to the single map instance.
       * Implementation akin to Meyers Singleton, see Alexandrescu, ch 6.4.
       */
      static HandleMap_& handleMapInstance_();
      static HandleTable_& handleTableInstance_();
      
      /**
       * Handle for the name represented by the Name object.
       */
      handle_t handle_;
  };
    
    std::ostream& operator<<(std::ostream&, const Name&);
    
    
    inline
    Name::HandleTable_& Name::handleTableInstance_()
    {
	// Meyers singleton, created first time function is invoked.
	
	static HandleTable_ handleTable(1,"0");
	
	return handleTable;
    }
    
    inline
    Name::HandleMap_& Name::handleMapInstance_()
    {
	// Meyers singleton, created first time function is invoked.
	static HandleMap_ handleMap;
	
	handleTableInstance_();
	
	return handleMap;
    }
    
    
} // end of namespace
#endif


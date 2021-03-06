/*
 *  lockptr.h
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

#ifndef SLI_LOCKPTR_H
#define SLI_LOCKPTR_H

#include <cassert>
#include <cstddef>
namespace sli3
{
/**
\class lockPTR

 This template is the standard safe-pointer implementation
 of NEST.

 In order for this scheme to work smoothly, the user has to take some
 precautions:
 1. each pointer should only be used ONCE to initialize a lockPTR.
 2. The lockPTR assumes that there are no other access points to the
    protected pointer.
 3. The lockPTR can freely be copied and passed between objects and functions.
 4. lockPTR objects should be used like the pointer to the object.
 5. lockPTR objects should be passed as objects in function calls and
    function return values.
 6. There should be no pointers to lockPTR objects.

 Class lockPTR is designed to behave just like the pointer would.  You
 can use the dereference operators (* and ->) to access the protected
 object. However, the pointer itself is (with exceptions) never
 exposed to the user.

 Since all access to the referenced object is done via a lockPTR, it
 is possible to maintain a count of all active references. If this
 count dropts to zero, the referenced object can savely be destroyed.
 For dynamically allocated objects, delete is envoked on the stored
 pointer.
 
 class lockPTR distinguishes between dynamically and automatically
 allocated objects by the way it is initialised: 

 If a lockPTR is initialised with a pointer, it assumes that the
 referenced object was dynamically allocated and will call the
 destructor once the reference count drops to zero.

 If the lockPTR is initialised with a reference, it assumes that the
 object is automatically allocated. Thus, the lockPTR wil NOT call the
 destructor.

 In some cases it is necessary for a routine to actually get hold of
 the pointer, contained in the lockPTR object.  This can be done by
 using the member function get().  After the pointer has been exposed
 this way, the lockPTR will regard the referenced object as unsafe,
 since the user might call delete on the pointer. Thus, lockPTR will
 "lock" the referenced object and deny all further access.
 The object can be unlocked by calling the unlock() member.
*/

template<class D>
class lockPTR
{  
  class PointerObject
  {
    
    private:

    D *pointee;                  // pointer to handled Datum object

    mutable 
      size_t number_of_references;

    bool deletable;
    bool locked;

    //!< forbid this constructor!
    PointerObject(PointerObject const&){assert(false);} 
    
    public:
    
    PointerObject(D* p = NULL)
      :pointee(p),
       number_of_references(1),
       deletable(true), locked(false){}
    
    PointerObject(D& p_o)
      :pointee(&p_o),
       number_of_references(1),
       deletable(false), locked(false){}

    ~PointerObject()
    { 
      assert(number_of_references == 0); // This will invalidate the still
                                         // existing pointer!      
      assert(!locked);
      if((pointee != NULL) && deletable && (!locked))
	delete pointee;
    }

    D * get(void) const
    {
      return pointee;
    }
    
    size_t add_reference(void) const
    {
      return ++number_of_references;
    }

    size_t remove_reference(void)
    {
      --number_of_references;
      if(number_of_references == 0)
      {
	delete this;
      }
      return number_of_references;
    }

    size_t references(void) const
    {
      return number_of_references;
    }
    
    bool islocked(void) const
      {
	return locked;
      }

    bool isdeletable(void) const
      {
	return deletable;
      }

    void lock(void)
      {
	locked = true;
      }

    void unlock(void)
      {
	locked = false;
      }
    
  };
    
  PointerObject *obj;

 public:
  
  // lockPTR() ; // generated automatically.
  // The default, argumentless constructor is used in
  // class declarations and generates an empty lockPTR
  // object which must then be initialised, for example 
  // by assignement.

  explicit lockPTR(D* p=NULL)
  {
    obj = new PointerObject(p);
    assert(obj != NULL);
  }
  
  explicit lockPTR(D& p_o)
  {
    obj = new PointerObject(p_o);
    assert(obj != NULL);
  }

  lockPTR(const lockPTR<D>& spd)
    : obj(spd.obj)
  {
    assert(obj != NULL);
    obj->add_reference();
  }
    
  virtual ~lockPTR()
  {
    assert(obj != NULL);
    obj->remove_reference();
  }    

  lockPTR<D> operator=(const lockPTR<D>&spd)
  {
    //  assert(obj != NULL);
    // assert(spd.obj != NULL);
	    
// The following order of the expressions protects
// against a=a;
    
    spd.obj->add_reference();
    obj->remove_reference();
	    
    obj = spd.obj;
	    
    return *this;
  }

  lockPTR<D> operator=(D &s)
    {
      *this = lockPTR<D>(s);
      return *this;
    }

  lockPTR<D> operator=(D const &s)
    {
      *this = lockPTR<D>(s);
      return *this;
    }

  D*  get(void)
  {
    assert(!obj->islocked());
    obj->lock(); // Try to lock Object
    return obj->get(); 
  }

  D*  get(void) const
  {
    assert(!obj->islocked());

    obj->lock(); // Try to lock Object
    return obj->get(); 
  }

  D*  operator->() const
  {
    assert(obj->get() != NULL);
    
    return obj->get(); 
  }
  
  D*  operator->()
  {
    assert(obj->get() != NULL);
    
    return obj->get(); 
  }

  D&  operator*()
  {
    assert(obj->get() != NULL);
    
    return *(obj->get()); 
  }
  
  const D&  operator*() const
  {
    assert(obj->get() != NULL);
    return *(obj->get()); 
  }
  
  
  bool operator!() const //!< returns true if and only if obj->pointee == NULL
  {
    // assert(obj != NULL);
    
    return (obj->get() == NULL);
  }

  bool operator==(const lockPTR<D>& p) const
  {
    return (obj == p.obj);
  }

  bool operator!=(const lockPTR<D>& p) const
  {
    return (obj != p.obj);
  }


  bool valid(void) const    //!< returns true if and only if obj->pointee != NULL
  {
    assert(obj != NULL);
    return (obj->get() != NULL);
  }

  bool islocked(void) const 
  {
    assert(obj != NULL);
    return (obj->islocked());
  }

  bool deletable(void) const 
  {
    assert(obj != NULL);
    return (obj->isdeletable());
  }

  void lock(void) const
    {
      assert(obj!=NULL);
      obj->lock();
    }

  void unlock(void) const
    {
      assert(obj!=NULL);
      obj->unlock();
    }

  void unlock(void)
    {
      assert(obj!=NULL);
      obj->unlock();
    }

  size_t references(void) const
    {
      return (obj==NULL)? 0: obj->references();
    }
};

}
#endif

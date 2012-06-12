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

#ifndef LOCK_PTR_H
#define LOCK_PTR_H

#ifdef NDEBUG
#define LOCK_PTR_NDEBUG
#endif

#include <cassert>
#include <cstddef>

/**
\class lockPTR

 This template is the standard safe-pointer implementation
 of SYNOD.

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

template <class D>
class RefObj
{
 protected:

  D data_;                  
  
  mutable size_t number_of_references_;

  bool locked_;         //!< object is const when locked_ is true.
  bool clone_on_write_; //!< If true, the object is cloned on write if multiple references exist.  
    
 public:
  RefObj();
  RefObj(D&);
  RefObj(const RefObj&);
  
  ~RefObj();
    
  size_t add_reference(void) const
  {
    return ++number_of_references_;
  }
  
  size_t remove_reference(void)
  {
    --number_of_references_;
    if(number_of_references_ == 0)
      {
	delete this;
      }
    return number_of_references_;
  }
  
    size_t references(void) const
    {
      return number_of_references_;
    }
    
    bool is_locked(void) const
      {
	return locked_;
      }

    bool clone_on_write(void) const
      {
	return clone_on_write_;
      }

    void lock(void)
      {
	locked_ = true;
      }

    void unlock(void)
      {
	locked_ = false;
      }

    const D& get(void) const
    {
      return data_;
    }

    D& get(void)
    {
      return data_;
    }
  };


template<class D>
class lockPtr
{  
    
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
    obj->addReference();
  }
    
  virtual ~lockPTR()
  {
    assert(obj != NULL);
    obj->removeReference();
  }    

  lockPTR<D> operator=(const lockPTR<D>&spd)
  {
    //  assert(obj != NULL);
    // assert(spd.obj != NULL);
	    
// The following order of the expressions protects
// against a=a;
    
    spd.obj->addReference();
    obj->removeReference();
	    
    obj = spd.obj;
	    
    return *this;
  }

  lockPTR<D> operator=(D &s)
    {
      *this = lockPTR<D>(s);
      assert(!(obj->isdeletable()));
      return *this;
    }

  lockPTR<D> operator=(D const &s)
    {
      *this = lockPTR<D>(s);
      assert(!(obj->isdeletable()));
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

#ifndef LOCK_PTR_NDEBUG
#undef NDEBUG
#endif

#endif

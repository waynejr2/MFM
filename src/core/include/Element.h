#ifndef ELEMENT_H /* -*- C++ -*- */
#define ELEMENT_H

#include "itype.h"
#include "EventWindow.h"
#include "Dirs.h"

namespace MFM
{
  typedef u32 ElementType;

  template <class C> class Atom; // Forward declaration

  /**
   * ELEMENT_EMPTY is recognized at the element/elementtable level.
   */
  static const u32 ELEMENT_EMPTY  = 0x0;

  template <class C>
  class Element
  {
  private:
    typedef typename C::ATOM_TYPE T;

  protected:
    static const SPoint VNNeighbors[4];

    bool FillAvailableVNNeighbor(EventWindow<C>& window, SPoint& pt) const;

    bool FillPointWithType(EventWindow<C>& window, 
			   SPoint& pt, const SPoint* relevants, u32 relevantCount,
			   Dir rotation, ElementType type) const;
      
    void Diffuse(EventWindow<C>& window) const;

  public:

    Element() 
    { }

    u32 GetType() const { return GetDefaultAtom().GetType(); }
    
    virtual const char * GetName() const = 0;

    virtual void Behavior(EventWindow<C>& window) const = 0;

    virtual const T & GetDefaultAtom() const = 0;

    virtual u32 DefaultPhysicsColor() const = 0;
    
    virtual u32 LocalPhysicsColor(const T &) const {
      return DefaultPhysicsColor();
    }
    
  };
}
  
#include "Element.tcc"

#endif /* ELEMENT_H */

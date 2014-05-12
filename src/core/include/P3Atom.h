#ifndef P3ATOM_H      /* -*- C++ -*- */
#define P3ATOM_H

#include <stdio.h>
#include "itype.h"
#include "Point.h"
#include "BitField.h"
#include "Atom.h"
#include "Element.h"
#include "CoreConfig.h"
#include "ParamConfig.h"
#include "Util.h"      /* For COMPILATION_REQUIREMENT */
#include "Parity2D_4x4.h"

namespace MFM {

  template <class PC>
  class P3Atom : public Atom< CoreConfig< P3Atom<PC>, PC> >
  {
    enum {
      BITS = PC::BITS_PER_ATOM,

      //////
      // P3 header configuration: Header is in low-bits end of the bitvector

      P3_ECC_BITS_POS = 0,
      P3_ECC_BITS_LEN = 9,

      P3_TYPE_BITS_POS = P3_ECC_BITS_POS + P3_ECC_BITS_LEN,
      P3_TYPE_BITS_LEN = 16,

      P3_FIXED_HEADER_POS = P3_ECC_BITS_POS,
      P3_FIXED_HEADER_LEN = P3_ECC_BITS_LEN + P3_TYPE_BITS_LEN,

      P3_STATE_BITS_POS = P3_FIXED_HEADER_POS + P3_FIXED_HEADER_LEN,
      P3_STATE_BITS_LEN = BITS - P3_STATE_BITS_POS,

      //////
      // Other constants

      P3_TYPE_COUNT = 1<<P3_TYPE_BITS_LEN

    };

    typedef BitField<BitVector<BITS>,P3_FIXED_HEADER_LEN,P3_FIXED_HEADER_POS> AFFixedHeader;
    typedef BitField<BitVector<BITS>,P3_TYPE_BITS_LEN,P3_TYPE_BITS_POS> AFTypeBits;
    typedef BitField<BitVector<BITS>,P3_ECC_BITS_LEN,P3_ECC_BITS_POS> AFECCBits;

  protected:

    /* We really don't want to allow the public to change the type of a
       P3Atom, since the type doesn't mean much without the atomic
       header as well */

    void SetType(u32 type) {

      if (type >= P3_TYPE_COUNT)
        FAIL(ILLEGAL_ARGUMENT);

      // Generate ECC and store all in header
      AFFixedHeader::Write(this->m_bits,Parity2D_4x4::Add2DParity(type));
    }

  public:

    P3Atom(u32 type = ELEMENT_EMPTY, u32 z1 = 0, u32 z2 = 0, u32 z3 = 0)
    {
      COMPILATION_REQUIREMENT< 32 <= BITS-1 >();

      if (z1 != 0 || z2 != 0 || z3 != 0)
        FAIL(ILLEGAL_ARGUMENT);

      SetType(type);
    }

    u32 GetType() const {
      return AFTypeBits::Read(this->m_bits);
    }

    bool IsSane() const
    {
      u32 fixedHeader = AFFixedHeader::Read(this->m_bits);
      return Parity2D_4x4::Check2DParity(fixedHeader);
    }

    u32 GetMaxStateSize(u32 type) const {
      return P3_STATE_BITS_LEN;
    }

    /**
     * Read stateWidth state bits starting at stateIndex, which counts
     * toward the right with 0 meaning the leftmost state bit.
     */
    u32 GetStateField(u32 stateIndex, u32 stateWidth) const
    {
      if (stateIndex + stateWidth > GetMaxStateSize(GetType()))
        FAIL(ILLEGAL_ARGUMENT);
      return this->m_bits.Read(P3_STATE_BITS_POS + stateIndex, stateWidth);
    }

    /**
     * Store value into stateWidth state bits starting at stateIndex,
     * which counts toward the right with 0 meaning the leftmost state
     * bit.
     */
    void SetStateField(u32 stateIndex, u32 stateWidth, u32 value)
    {
      if (stateIndex + stateWidth > GetMaxStateSize(GetType()))
        FAIL(ILLEGAL_ARGUMENT);
      return this->m_bits.Write(P3_STATE_BITS_POS + stateIndex, stateWidth, value);
    }

    void PrintBits(FILE* ostream) const
    { this->m_bits.Print(ostream); }

    void Print(FILE* ostream) const
    {
      u32 type = GetType();
      fprintf(ostream,"P3[%x/",type);
      u32 length = GetMaxStateSize(type);
      for (int i = 0; i < length; i += 4) {
        u32 nyb = this->m_bits.Read(i,4);
        fprintf(ostream,"%x",nyb);
      }
      fprintf(ostream,"]");
    }

    P3Atom& operator=(const P3Atom & rhs)
    {
      if (this == &rhs) return *this;

      this->m_bits = rhs.m_bits;

      return *this;
    }

  };
} /* namespace MFM */

#endif /*P3ATOM_H*/


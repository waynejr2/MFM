/*                                              -*- mode:C++ -*-
  OverflowableCharBufferByteSink.h Overflowable character stream
  Copyright (C) 2014 The Regents of the University of New Mexico.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
  USA
*/

/**
  \file OverflowableCharBufferByteSink.h Overflowable character stream
  \author David H. Ackley.
  \date (C) 2014 All rights reserved.
  \lgpl
 */
#ifndef OVERFLOWABLECHARBUFFERBYTESINK_H
#define OVERFLOWABLECHARBUFFERBYTESINK_H

#include "ByteSink.h"
#include <string.h>        /* For memcpy */

namespace MFM {

  /**
   * A ByteSink that holds up to BUFSIZE - 2 data bytes.  If that
   * limit is reached, an 'X' is appended at the end of the ByteSink,
   * and further data written is discarded.
   */
  template <u32 BUFSIZE>
  class OverflowableCharBufferByteSink : public ByteSink {
  public:
    OverflowableCharBufferByteSink() : m_written(0), m_overflowed(false) { }

    virtual void WriteBytes(const u8 * data, const u32 len) {

      if (m_overflowed)
        return;

      u32 effLen = len;
      if (m_written + effLen > BUFSIZE - 2) {
        effLen = BUFSIZE - 2 - m_written;
        m_overflowed = true;
      }

      memcpy(&m_buf[m_written], data, effLen);
      m_written += effLen;

      if (m_overflowed)
        m_buf[m_written++] = 'X';
    }

    virtual s32 CanWrite() {
      return BUFSIZE - m_written - 1;
    }

    bool Equals(const char * str) const {
      return GetLength() == strlen(str) && !memcmp(m_buf, str, GetLength());
    }

    template <u32 OTHER_SIZE>
    bool Equals(const OverflowableCharBufferByteSink<OTHER_SIZE> & str) const {
      return GetLength() == str.GetLength() && !memcmp(m_buf, str.m_buf, GetLength());
    }

    const char * GetZString() {
      m_buf[m_written] = '\0';
      return (const char *) m_buf;
    }

    u32 GetLength() const {
      return m_written;
    }

    u32 GetCapacity() const {
      return BUFSIZE;
    }

    void Reset() {
      m_written = 0;
      m_overflowed = false;
    }

    bool HasOverflowed() const {
      return m_overflowed;
    }

  private:
    u32 m_written;
    bool m_overflowed;
    u8 m_buf[BUFSIZE];
  };

  typedef OverflowableCharBufferByteSink<16 + 2> OString16;
  typedef OverflowableCharBufferByteSink<32 + 2> OString32;
  typedef OverflowableCharBufferByteSink<64 + 2> OString64;
  typedef OverflowableCharBufferByteSink<128 + 2> OString128;
}


#endif /* OVERFLOWABLECHARBUFFERBYTESINK_H */
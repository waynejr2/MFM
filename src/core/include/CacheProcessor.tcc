/* -*- C++ -*- */

#include "PacketIO.h"

namespace MFM
{
  template <class CC>
  void CacheProcessor<CC>::ReportCacheProcessorStatus(Logger::Level level)
  {
    if (!m_tile)
    {
      LOG.Log(level,"   ==CP Global NO TILE (%d, %d)==",
              m_farSideOrigin.GetX(),
              m_farSideOrigin.GetY());
      return;
    }
    LOG.Log(level,"   ==CP Global %s (%d, %d)==",
            m_tile->GetLabel(),
            m_farSideOrigin.GetX(),
            m_farSideOrigin.GetY());
    LOG.Log(level,"    Address: %p", (void*) this);
    LOG.Log(level,"    State: %s", GetStateName(m_cpState));
    LOG.Log(level,"    EventCenter: (%d,%d)", m_eventCenter.GetX(), m_eventCenter.GetY());
    LOG.Log(level,"    CheckOdds: %d", m_checkOdds);
    LOG.Log(level,"    ToSendCount: %d", m_toSendCount);
    LOG.Log(level,"    SentCount:   %d", m_sentCount);

    m_channelEnd.ReportChannelEndStatus(level);
  }

  template <class CC>
  bool CacheProcessor<CC>::IsSiteNumberVisible(u16 siteNumber)
  {
    const MDist<R> & md = MDist<R>::get();

    // Get site address in full untransformed tile coordinates
    SPoint local = md.GetPoint(siteNumber) + m_eventCenter;
    return IsCoordVisibleToPeer(local);
  }

  template <class CC>
  bool CacheProcessor<CC>::IsCoordVisibleToPeer(const SPoint & local)
  {
    // Map full untransformed local tile coord into the remote space
    SPoint remote = LocalToRemote(local);

    // Get its distance from the remote tile center
    u32 dist = Tile<CC>::GetSquareDistanceFromCenter(remote);

    // Distances up to a tile radius are visible
    bool visible = dist <= TILE_WIDTH / 2;

    // Which is what you asked
    return visible;
  }

  template <class CC>
  void CacheProcessor<CC>::StartShipping()
  {
    if (m_cpState != LOADING)
    {
      FAIL(ILLEGAL_STATE);
    }

    // Now it's about shipping
    SetStateInternal(SHIPPING);

    PacketIO pbuffer;
    if (!pbuffer.SendUpdateBegin(*this, m_eventCenter))
    {
      FAIL(ILLEGAL_STATE);
    }
  }

  template <class CC>
  void CacheProcessor<CC>::StartLoading(const SPoint & eventCenter)
  {
    if (m_cpState != ACTIVE)
    {
      FAIL(ILLEGAL_STATE);
    }

    SetStateInternal(LOADING);
    m_eventCenter = eventCenter;
    m_toSendCount = 0;
    m_sentCount = 0;
  }

  template <class CC>
  void CacheProcessor<CC>::MaybeSendAtom(const T & atom, bool changed, u16 siteNumber)
  {
    if (m_cpState != LOADING)
    {
      FAIL(ILLEGAL_STATE);
    }

    // If far side can't see it, done
    if (!IsSiteNumberVisible(siteNumber))
    {
      return;
    }

    Tile<CC> & t = GetTile();
    Random & random = t.GetRandom();
    const u32 checkOdds = GetCheckOdds();

    // If unchanged and not time for a redundant check, done
    if (!changed && !random.OneIn(checkOdds))
    {
      return;
    }

    // Time to pack this puppy up for travel
    if (m_toSendCount >= SITE_COUNT)
    {
      FAIL(ILLEGAL_STATE);  // You say ship more than a whole window?
    }

    MFM_LOG_DBG7(("CP %s (%d,%d) send #%d (%d,%d)",
                  m_tile->GetLabel(),
                  m_farSideOrigin.GetX(),
                  m_farSideOrigin.GetY(),
                  siteNumber,
                  99,
                  99));

    // Allocate next struct
    CachePacketInfo & cpi = m_toSend[m_toSendCount++];

    cpi.m_atom = atom;
    cpi.m_siteNumber = siteNumber;
    cpi.m_type = changed ? PacketType::UPDATE : PacketType::CHECK;
  }

  template <class CC>
  bool CacheProcessor<CC>::ShipBufferAsPacket(PacketBuffer & pb)
  {
    if (pb.HasOverflowed())
    {
      FAIL(OUT_OF_ROOM);
    }

    u32 plen = pb.GetLength();
    if (m_channelEnd.CanWrite() <= plen) // Total write will be plen+1
    {
      return false;
    }

    u8 byte = (u8) plen;  // plen<128 since OString128..
    m_channelEnd.Write(&byte, 1);  // Packet length, then data
    m_channelEnd.Write((const u8 *) pb.GetBuffer(), plen);
    return true;
  }

  template <class CC>
  void CacheProcessor<CC>::BeginUpdate(SPoint onCenter)
  {
    if (m_cpState != IDLE)
    {
      FAIL(ILLEGAL_STATE);
    }

    SetStateInternal(PASSIVE);
    m_eventCenter = onCenter;
    m_consistentAtomCount = 0;  // Ready for my update..
  }

  template <class CC>
  void CacheProcessor<CC>::ReceiveAtom(bool isDifferent, s32 siteNumber, const T & inboundAtom)
  {
    if (m_cpState != PASSIVE || m_tile == 0)
    {
      FAIL(ILLEGAL_STATE);
    }

    if (siteNumber < 0 || siteNumber >= SITE_COUNT)
    {
      FAIL(ILLEGAL_ARGUMENT);
    }

    if (!IsSiteNumberVisible((u16) siteNumber))
    {
      FAIL(ILLEGAL_ARGUMENT);
    }

    const MDist<R> & md = MDist<R>::get();
    SPoint loc = md.GetPoint(siteNumber) + m_eventCenter;

    bool consistent =
      m_tile->ApplyCacheUpdate(isDifferent, inboundAtom, loc);

    if (consistent)
    {
      ++m_consistentAtomCount;
    }

    MFM_LOG_DBG6(("CP %s (%d,%d) %s site #%d arrived for (%d, %d) -> %s",
                  m_tile->GetLabel(),
                  m_farSideOrigin.GetX(),
                  m_farSideOrigin.GetY(),
                  isDifferent? "Updated" : "Check",
                  siteNumber,
                  loc.GetX(),
                  loc.GetY(),
                  consistent ? "Consistent" : "Inconsistent"));
  }

  template <class CC>
  void CacheProcessor<CC>::ReceiveUpdateEnd()
  {
    if (m_cpState != PASSIVE)
    {
      FAIL(ILLEGAL_STATE);
    }
    MFM_LOG_DBG7(("Replying to UE, %d consistent",
                  m_consistentAtomCount));
    PacketIO pbuffer;
    pbuffer.SendReply(m_consistentAtomCount, *this);
    SetIdle();
  }

  template <class CC>
  void CacheProcessor<CC>::ReceiveReply(u32 consistentCount)
  {
    if (m_cpState != RECEIVING)
    {
      FAIL(ILLEGAL_STATE);
    }
    if (consistentCount != m_toSendCount)
    {
      ReportCheckFailure();
    }
    else
    {
      ReportCleanUpdate();
    }
    MFM_LOG_DBG7(("CP %s (%d,%d) reply %d<->%d : %d",
              m_tile->GetLabel(),
              m_farSideOrigin.GetX(),
              m_farSideOrigin.GetY(),
              consistentCount,
              m_toSendCount,
                  m_checkOdds));
    Unlock(); // Finally!
    SetIdle();
  }

  template <class CC>
  bool CacheProcessor<CC>::Advance()
  {
    // If we're unconnected, we're not working
    if (!m_tile)
    {
      return false;
    }

    MFM_LOG_DBG7(("CP %s (%d,%d) Advance in state %s",
                  m_tile->GetLabel(),
                  m_farSideOrigin.GetX(),
                  m_farSideOrigin.GetY(),
                  GetStateName(m_cpState)));

    switch (m_cpState)
    {
    case SHIPPING: return AdvanceShipping();
    case IDLE:    // During idle we just receive
    case PASSIVE: // During passive we receive
    case RECEIVING: return AdvanceReceiving();
    default:
      FAIL(ILLEGAL_STATE);
    }
  }

  template <class CC>
  void CacheProcessor<CC>::Activate()
  {
    if (m_cpState != IDLE)
    {
      FAIL(ILLEGAL_STATE);
    }
    SetStateInternal(ACTIVE);
  }

  template <class CC>
  void CacheProcessor<CC>::SetIdle()
  {
    if (m_cpState == IDLE)
    {
      FAIL(ILLEGAL_STATE);
    }
    SetStateInternal(IDLE);
  }

  template <class CC>
  bool CacheProcessor<CC>::AdvanceShipping()
  {
    MFM_LOG_DBG7(("CP %s (%d,%d): Advance shipping",
                  m_tile->GetLabel(),
                  m_farSideOrigin.GetX(),
                  m_farSideOrigin.GetY()));
    bool didWork = false;
    PacketIO pbuffer;

    // Try to send any unsent packets
    while (m_sentCount < m_toSendCount)
    {
      CachePacketInfo & cpi = m_toSend[m_sentCount];
      if (!pbuffer.SendAtom(cpi.m_type, *this, cpi.m_siteNumber, cpi.m_atom))
      {
        return didWork;
      }
      didWork = true;
      ++m_sentCount;
      MFM_LOG_DBG7(("CP %s (%d,%d): Ship %d (site #%d)",
                    m_tile->GetLabel(),
                    m_farSideOrigin.GetX(),
                    m_farSideOrigin.GetY(),
                    m_sentCount,
                    cpi.m_siteNumber));
    }

    // Try to send the update end packet if not yet sent
    if (m_sentCount == m_toSendCount)
    {
      if (!pbuffer.SendUpdateEnd(*this))
      {
        return didWork;
      }
      didWork = true;
    }

    SetStateInternal(RECEIVING);
    return didWork;
  }

  template <class CC>
  bool CacheProcessor<CC>::AdvanceReceiving()
  {
    bool didWork = false;
    if (!IsConnected())
    {
      return didWork;
    }

    PacketIO pio;
    while (true)
    {
      PacketBuffer * pb = m_channelEnd.ReceivePacket();
      if (!pb || pb->GetLength() == 0)
      {
        return didWork;
      }
      didWork = true;
      if (!pio.HandlePacket(*this, *pb))
      {
        FAIL(INCOMPLETE_CODE);
      }
    }
  }


#if 0
    ChannelEnd & cxn = m_channelEnd;
    PacketIO pbuffer;

      while (cui.m_atomsConsidered < SITE_COUNT)
      {
        bool dirty = m_isDirtySite[cui.m_atomsConsidered];
        if (dirty || random.OneIn(checkOdds))
        {
          if (!pbuffer.SendAtom<CC>(dirty ? PACKET_UPDATE : PACKET_CHECK,
                                    cxn, cui.m_atomsConsidered,
                                    m_atomBuffer[cui.m_atomsConsidered]))
          {
            break; // No room.  Try again later; don't advance m_atomsConsidered
          }
          else
          {
            didWork |= true;
          }
        }
        ++cui.m_atomsConsidered;
      }
    }
    return didWork;
  }
#endif

}
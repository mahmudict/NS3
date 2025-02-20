/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "ns3/log.h"
#include "ns3/pointer.h"
#include "ns3/packet.h"
#include "regular-wifi-mac.h"
#include "wifi-phy.h"
#include "mac-rx-middle.h"
#include "mac-tx-middle.h"
#include "mac-low.h"
#include "msdu-aggregator.h"
#include "mpdu-aggregator.h"
#include "wifi-utils.h"
#include "mgt-headers.h"
#include "amsdu-subframe-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RegularWifiMac");

NS_OBJECT_ENSURE_REGISTERED (RegularWifiMac);

RegularWifiMac::RegularWifiMac ()
  : m_qosSupported (0),
    m_htSupported (0),
    m_vhtSupported (0),
    m_erpSupported (0),
    m_dsssSupported (0),
    m_heSupported (0)
{
  NS_LOG_FUNCTION (this);
  m_rxMiddle = Create<MacRxMiddle> ();
  m_rxMiddle->SetForwardCallback (MakeCallback (&RegularWifiMac::Receive, this));

  m_txMiddle = Create<MacTxMiddle> ();

  m_low = CreateObject<MacLow> ();
  m_low->SetRxCallback (MakeCallback (&MacRxMiddle::Receive, m_rxMiddle));

  m_channelAccessManager = CreateObject<ChannelAccessManager> ();
  m_channelAccessManager->SetupLow (m_low);

  m_txop = CreateObject<Txop> ();
  m_txop->SetMacLow (m_low);
  m_txop->SetChannelAccessManager (m_channelAccessManager);
  m_txop->SetTxMiddle (m_txMiddle);
  m_txop->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  m_txop->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  m_txop->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));

  //Construct the EDCAFs. The ordering is important - highest
  //priority (Table 9-1 UP-to-AC mapping; IEEE 802.11-2012) must be created
  //first.
  SetupEdcaQueue (AC_DVO); //                       //changed in this area
  SetupEdcaQueue (AC_VO); // voice
  SetupEdcaQueue (AC_KVI);                //
  SetupEdcaQueue (AC_DVI); //
  SetupEdcaQueue (AC_VI); // video
  SetupEdcaQueue (AC_BE);
  SetupEdcaQueue (AC_BK);
}

RegularWifiMac::~RegularWifiMac ()
{
  NS_LOG_FUNCTION (this);
}

void
RegularWifiMac::DoInitialize ()
{
  NS_LOG_FUNCTION (this);
  m_txop->Initialize ();

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Initialize ();
    }
}

void
RegularWifiMac::DoDispose ()
{
  NS_LOG_FUNCTION (this);

  m_rxMiddle = 0;
  m_txMiddle = 0;

  m_low->Dispose ();
  m_low = 0;

  m_phy = 0;
  m_stationManager = 0;

  m_txop->Dispose ();
  m_txop = 0;

  for (EdcaQueues::iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->Dispose ();
      i->second = 0;
    }

  m_channelAccessManager->Dispose ();
  m_channelAccessManager = 0;
}

void
RegularWifiMac::SetWifiRemoteStationManager (const Ptr<WifiRemoteStationManager> stationManager)
{
  NS_LOG_FUNCTION (this << stationManager);
  m_stationManager = stationManager;
  m_stationManager->SetHtSupported (GetHtSupported ());
  m_stationManager->SetVhtSupported (GetVhtSupported ());
  m_stationManager->SetHeSupported (GetHeSupported ());
  m_low->SetWifiRemoteStationManager (stationManager);

  m_txop->SetWifiRemoteStationManager (stationManager);

  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetWifiRemoteStationManager (stationManager);
    }
}

Ptr<WifiRemoteStationManager>
RegularWifiMac::GetWifiRemoteStationManager () const
{
  return m_stationManager;
}

ExtendedCapabilities
RegularWifiMac::GetExtendedCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  ExtendedCapabilities capabilities;
  if (m_htSupported || m_vhtSupported)
    {
      if (m_htSupported)
        {
          capabilities.SetHtSupported (1);
        }
      if (m_vhtSupported)
        {
          capabilities.SetVhtSupported (1);
        }
    }
  //TODO: to be completed
  return capabilities;
}

HtCapabilities
RegularWifiMac::GetHtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HtCapabilities capabilities;
  if (m_htSupported)
    {
      capabilities.SetHtSupported (1);
      capabilities.SetLdpc (m_phy->GetLdpc ());
      capabilities.SetSupportedChannelWidth (m_phy->GetChannelWidth () >= 40);
      capabilities.SetShortGuardInterval20 (m_phy->GetShortGuardInterval ());
      capabilities.SetShortGuardInterval40 (m_phy->GetChannelWidth () >= 40 && m_phy->GetShortGuardInterval ());
      capabilities.SetGreenfield (m_phy->GetGreenfield ());
      uint32_t maxAmsduLength = std::max (std::max (m_beMaxAmsduSize, m_bkMaxAmsduSize), std::max (m_voMaxAmsduSize, m_viMaxAmsduSize));
      capabilities.SetMaxAmsduLength (maxAmsduLength > 3839); //0 if 3839 and 1 if 7935
      capabilities.SetLSigProtectionSupport (!m_phy->GetGreenfield ());
      double maxAmpduLengthExponent = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize))
                                                                      + 1.0)
                                                            / std::log (2.0))
                                                           - 13.0),
                                                0.0);
      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities.SetMaxAmpduLength (std::max<uint8_t> (3, static_cast<uint8_t> (maxAmpduLengthExponent))); //0 to 3 for HT
      uint64_t maxSupportedRate = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_HT)
            {
              continue;
            }
          capabilities.SetRxMcsBitmask (mcs.GetMcsValue ());
          uint8_t nss = (mcs.GetMcsValue () / 8) + 1;
          NS_ASSERT (nss > 0 && nss < 5);
          uint64_t dataRate = mcs.GetDataRate (m_phy->GetChannelWidth (), m_phy->GetShortGuardInterval () ? 400 : 800, nss);
          if (dataRate > maxSupportedRate)
            {
              maxSupportedRate = dataRate;
              NS_LOG_DEBUG ("Updating maxSupportedRate to " << maxSupportedRate);
            }
        }
      capabilities.SetRxHighestSupportedDataRate (static_cast<uint16_t> (maxSupportedRate / 1e6)); //in Mbit/s
      capabilities.SetTxMcsSetDefined (m_phy->GetNMcs () > 0);
      capabilities.SetTxMaxNSpatialStreams (m_phy->GetMaxSupportedTxSpatialStreams ());
      //we do not support unequal modulations
      capabilities.SetTxRxMcsSetUnequal (0);
      capabilities.SetTxUnequalModulation (0);
    }
  return capabilities;
}

VhtCapabilities
RegularWifiMac::GetVhtCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  VhtCapabilities capabilities;
  if (m_vhtSupported)
    {
      capabilities.SetVhtSupported (1);
      if (m_phy->GetChannelWidth () == 160)
        {
          capabilities.SetSupportedChannelWidthSet (1);
        }
      else
        {
          capabilities.SetSupportedChannelWidthSet (0);
        }
      uint32_t maxMpduLength = std::max (std::max (m_beMaxAmsduSize, m_bkMaxAmsduSize), std::max (m_voMaxAmsduSize, m_viMaxAmsduSize)) + 56; //see section 9.11 of 11ac standard
      capabilities.SetMaxMpduLength (uint8_t (maxMpduLength > 3895) + uint8_t (maxMpduLength > 7991)); //0 if 3895, 1 if 7991, 2 for 11454
      capabilities.SetRxLdpc (m_phy->GetLdpc ());
      capabilities.SetShortGuardIntervalFor80Mhz ((m_phy->GetChannelWidth () == 80) && m_phy->GetShortGuardInterval ());
      capabilities.SetShortGuardIntervalFor160Mhz ((m_phy->GetChannelWidth () == 160) && m_phy->GetShortGuardInterval ());
      double maxAmpduLengthExponent = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize)) + 1.0) / std::log (2.0)) - 13.0), 0.0);
      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities.SetMaxAmpduLengthExponent (std::max<uint8_t> (7, static_cast<uint8_t> (maxAmpduLengthExponent))); //0 to 7 for VHT
      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_VHT)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      // Support same MaxMCS for each spatial stream
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedRxSpatialStreams (); nss++)
        {
          capabilities.SetRxMcsMap (maxMcs, nss);
        }
      for (uint8_t nss = 1; nss <= m_phy->GetMaxSupportedTxSpatialStreams (); nss++)
        {
          capabilities.SetTxMcsMap (maxMcs, nss);
        }
      uint64_t maxSupportedRateLGI = 0; //in bit/s
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if (mcs.GetModulationClass () != WIFI_MOD_CLASS_VHT || !mcs.IsAllowed (m_phy->GetChannelWidth (), 1))
            {
              continue;
            }
          if (mcs.GetDataRate (m_phy->GetChannelWidth ()) > maxSupportedRateLGI)
            {
              maxSupportedRateLGI = mcs.GetDataRate (m_phy->GetChannelWidth ());
              NS_LOG_DEBUG ("Updating maxSupportedRateLGI to " << maxSupportedRateLGI);
            }
        }
      capabilities.SetRxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      capabilities.SetTxHighestSupportedLgiDataRate (static_cast<uint16_t> (maxSupportedRateLGI / 1e6)); //in Mbit/s
      //To be filled in once supported
      capabilities.SetRxStbc (0);
      capabilities.SetTxStbc (0);
    }
  return capabilities;
}

HeCapabilities
RegularWifiMac::GetHeCapabilities (void) const
{
  NS_LOG_FUNCTION (this);
  HeCapabilities capabilities;
  if (m_heSupported)
    {
      capabilities.SetHeSupported (1);
      uint8_t channelWidthSet = 0;
      if (m_phy->GetChannelWidth () >= 40 && Is2_4Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x01;
        }
      if (m_phy->GetChannelWidth () >= 80 && Is5Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x02;
        }
      if (m_phy->GetChannelWidth () >= 160 && Is5Ghz (m_phy->GetFrequency ()))
        {
          channelWidthSet |= 0x04;
        }
      capabilities.SetChannelWidthSet (channelWidthSet);
      uint8_t gi = 0;
      if (m_phy->GetGuardInterval () <= NanoSeconds (1600))
        {
          //todo: We assume for now that if we support 800ns GI then 1600ns GI is supported as well
          gi |= 0x01;
        }
      if (m_phy->GetGuardInterval () == NanoSeconds (800))
        {
          gi |= 0x02;
        }
      capabilities.SetHeLtfAndGiForHePpdus (gi);

      //The ceil() function in C++ returns the smallest possible integer value which is greater than or equal to the given argument
      double maxAmpduLengthExponent1 = std::max (std::ceil ((std::log (std::max (std::max (m_beMaxAmpduSize, m_bkMaxAmpduSize), std::max (m_voMaxAmpduSize, m_viMaxAmpduSize))
                                                                      + 1.0)
                                                            / std::log (2.0))
                                                           - 13.0),
                                                0.0);
      double maxAmpduLengthExponent2 = std::max (std::ceil ((std::log (std::max (std::max (m_dvoMaxAmpduSize, m_voMaxAmpduSize), std::max (m_kviMaxAmpduSize, m_dviMaxAmpduSize))
                                                                      + 1.0)
                                                            / std::log (2.0))
                                                           - 13.0),
                                                0.0);
     double maxAmpduLengthExponent = std::max(maxAmpduLengthExponent1, maxAmpduLengthExponent2);

      NS_ASSERT (maxAmpduLengthExponent >= 0 && maxAmpduLengthExponent <= 255);
      capabilities.SetMaxAmpduLengthExponent (std::max<uint8_t> (7, static_cast<uint8_t> (maxAmpduLengthExponent))); //assume 0 to 7 for HE
      uint8_t maxMcs = 0;
      for (uint8_t i = 0; i < m_phy->GetNMcs (); i++)
        {
          WifiMode mcs = m_phy->GetMcs (i);
          if ((mcs.GetModulationClass () == WIFI_MOD_CLASS_HE)
              && (mcs.GetMcsValue () > maxMcs))
            {
              maxMcs = mcs.GetMcsValue ();
            }
        }
      capabilities.SetHighestMcsSupported (maxMcs);
      capabilities.SetHighestNssSupported (m_phy->GetMaxSupportedTxSpatialStreams ());
    }
  return capabilities;
}

void
RegularWifiMac::SetDvoMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_dvoMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetVoMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_voMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetKviMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_kviMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetDviMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_dviMaxAmsduSize = size;
  ConfigureAggregation ();
}
void
RegularWifiMac::SetViMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_viMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBeMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_beMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBkMaxAmsduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_bkMaxAmsduSize = size;
  ConfigureAggregation ();
}

////////////////////////////////////////////

void
RegularWifiMac::SetDvoMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_dvoMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetVoMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_voMaxAmsduSize = size;
  ConfigureAggregation ();
}


void
RegularWifiMac::SetKviMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_kviMaxAmpduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetDviMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_dviMaxAmpduSize = size;
  ConfigureAggregation ();
}
void
RegularWifiMac::SetViMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_viMaxAmsduSize = size;
  ConfigureAggregation ();
}

void
RegularWifiMac::SetBkMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_bkMaxAmsduSize = size;
  ConfigureAggregation ();
}
void
RegularWifiMac::SetBeMaxAmpduSize (uint16_t size)
{
  NS_LOG_FUNCTION (this << size);
  m_beMaxAmpduSize = size;
  ConfigureAggregation ();
}

///////////////////////////////////////////////////////////////////

void
RegularWifiMac::SetDvoBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetDVOQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetVoBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVOQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetKviBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetKVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetDviBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetDVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetViBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetVIQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBeBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBEQueue ()->SetBlockAckThreshold (threshold);
}

void
RegularWifiMac::SetBkBlockAckThreshold (uint8_t threshold)
{
  NS_LOG_FUNCTION (this << +threshold);
  GetBKQueue ()->SetBlockAckThreshold (threshold);
}
/////////////////////////////////////////////////////


void
RegularWifiMac::SetDvoBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetDVOQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetVoBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVOQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetKviBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetKVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}
void
RegularWifiMac::SetDviBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetDVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetViBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetVIQueue ()->SetBlockAckInactivityTimeout (timeout);
}


void
RegularWifiMac::SetBeBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBEQueue ()->SetBlockAckInactivityTimeout (timeout);
}

void
RegularWifiMac::SetBkBlockAckInactivityTimeout (uint16_t timeout)
{
  NS_LOG_FUNCTION (this << timeout);
  GetBKQueue ()->SetBlockAckInactivityTimeout (timeout);
}

/////////////////////////////////////////////////////////////////////////

void
RegularWifiMac::SetupEdcaQueue (AcIndex ac)
{
  NS_LOG_FUNCTION (this << ac);

  //Our caller shouldn't be attempting to setup a queue that is
  //already configured.
  NS_ASSERT (m_edca.find (ac) == m_edca.end ());

  Ptr<QosTxop> edca = CreateObject<QosTxop> ();
  edca->SetMacLow (m_low);
  edca->SetChannelAccessManager (m_channelAccessManager);
  edca->SetTxMiddle (m_txMiddle);
  edca->SetTxOkCallback (MakeCallback (&RegularWifiMac::TxOk, this));
  edca->SetTxFailedCallback (MakeCallback (&RegularWifiMac::TxFailed, this));
  edca->SetTxDroppedCallback (MakeCallback (&RegularWifiMac::NotifyTxDrop, this));
  edca->SetAccessCategory (ac);
  edca->CompleteConfig ();

  m_edca.insert (std::make_pair (ac, edca));
}

void
RegularWifiMac::SetTypeOfStation (TypeOfStation type)
{
  NS_LOG_FUNCTION (this << type);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetTypeOfStation (type);
    }
}

Ptr<Txop>
RegularWifiMac::GetTxop () const
{
  return m_txop;
}

Ptr<QosTxop>
RegularWifiMac::GetDVOQueue () const
{
  return m_edca.find (AC_DVO)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetVOQueue () const
{
  return m_edca.find (AC_VO)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetKVIQueue () const
{
  return m_edca.find (AC_KVI)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetDVIQueue () const
{
  return m_edca.find (AC_DVI)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetVIQueue () const
{
  return m_edca.find (AC_VI)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBEQueue () const
{
  return m_edca.find (AC_BE)->second;
}

Ptr<QosTxop>
RegularWifiMac::GetBKQueue () const
{
  return m_edca.find (AC_BK)->second;
}
////////////////////////////////////////////////////
void
RegularWifiMac::SetWifiPhy (const Ptr<WifiPhy> phy)
{
  NS_LOG_FUNCTION (this << phy);
  m_phy = phy;
  m_channelAccessManager->SetupPhyListener (phy);
  m_low->SetPhy (phy);
}

Ptr<WifiPhy>
RegularWifiMac::GetWifiPhy (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phy;
}

void
RegularWifiMac::ResetWifiPhy (void)
{
  NS_LOG_FUNCTION (this);
  m_low->ResetPhy ();
  m_channelAccessManager->RemovePhyListener (m_phy);
  m_phy = 0;
}

void
RegularWifiMac::SetForwardUpCallback (ForwardUpCallback upCallback)
{
  NS_LOG_FUNCTION (this);
  m_forwardUp = upCallback;
}

void
RegularWifiMac::SetLinkUpCallback (Callback<void> linkUp)
{
  NS_LOG_FUNCTION (this);
  m_linkUp = linkUp;
}

void
RegularWifiMac::SetLinkDownCallback (Callback<void> linkDown)
{
  NS_LOG_FUNCTION (this);
  m_linkDown = linkDown;
}

void
RegularWifiMac::SetQosSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_qosSupported = enable;
}

bool
RegularWifiMac::GetQosSupported () const
{
  return m_qosSupported;
}

void
RegularWifiMac::SetVhtSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_vhtSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_htSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

void
RegularWifiMac::SetHtSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_htSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_vhtSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

void
RegularWifiMac::SetHeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_heSupported = enable;
  if (enable)
    {
      SetQosSupported (true);
    }
  if (!enable && !m_htSupported && !m_vhtSupported)
    {
      DisableAggregation ();
    }
  else
    {
      EnableAggregation ();
    }
}

bool
RegularWifiMac::GetVhtSupported () const
{
  return m_vhtSupported;
}

bool
RegularWifiMac::GetHtSupported () const
{
  return m_htSupported;
}

bool
RegularWifiMac::GetHeSupported () const
{
  return m_heSupported;
}

bool
RegularWifiMac::GetErpSupported () const
{
  return m_erpSupported;
}

void
RegularWifiMac::SetErpSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  if (enable)
    {
      SetDsssSupported (true);
    }
  m_erpSupported = enable;
}

void
RegularWifiMac::SetDsssSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_dsssSupported = enable;
}

bool
RegularWifiMac::GetDsssSupported () const
{
  return m_dsssSupported;
}

void
RegularWifiMac::SetCtsToSelfSupported (bool enable)
{
  NS_LOG_FUNCTION (this);
  m_low->SetCtsToSelfSupported (enable);
}

void
RegularWifiMac::SetSlot (Time slotTime)
{
  NS_LOG_FUNCTION (this << slotTime);
  m_channelAccessManager->SetSlot (slotTime);
  m_low->SetSlotTime (slotTime);
}

Time
RegularWifiMac::GetSlot (void) const
{
  return m_low->GetSlotTime ();
}

void
RegularWifiMac::SetSifs (Time sifs)
{
  NS_LOG_FUNCTION (this << sifs);
  m_channelAccessManager->SetSifs (sifs);
  m_low->SetSifs (sifs);
}

Time
RegularWifiMac::GetSifs (void) const
{
  return m_low->GetSifs ();
}

void
RegularWifiMac::SetEifsNoDifs (Time eifsNoDifs)
{
  NS_LOG_FUNCTION (this << eifsNoDifs);
  m_channelAccessManager->SetEifsNoDifs (eifsNoDifs);
}

Time
RegularWifiMac::GetEifsNoDifs (void) const
{
  return m_channelAccessManager->GetEifsNoDifs ();
}

void
RegularWifiMac::SetRifs (Time rifs)
{
  NS_LOG_FUNCTION (this << rifs);
  m_low->SetRifs (rifs);
}

Time
RegularWifiMac::GetRifs (void) const
{
  return m_low->GetRifs ();
}

void
RegularWifiMac::SetPifs (Time pifs)
{
  NS_LOG_FUNCTION (this << pifs);
  m_low->SetPifs (pifs);
}

Time
RegularWifiMac::GetPifs (void) const
{
  return m_low->GetPifs ();
}

void
RegularWifiMac::SetAckTimeout (Time ackTimeout)
{
  NS_LOG_FUNCTION (this << ackTimeout);
  m_low->SetAckTimeout (ackTimeout);
}

Time
RegularWifiMac::GetAckTimeout (void) const
{
  return m_low->GetAckTimeout ();
}

void
RegularWifiMac::SetCtsTimeout (Time ctsTimeout)
{
  NS_LOG_FUNCTION (this << ctsTimeout);
  m_low->SetCtsTimeout (ctsTimeout);
}

Time
RegularWifiMac::GetCtsTimeout (void) const
{
  return m_low->GetCtsTimeout ();
}

void
RegularWifiMac::SetBasicBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetBasicBlockAckTimeout (blockAckTimeout);
}

Time
RegularWifiMac::GetBasicBlockAckTimeout (void) const
{
  return m_low->GetBasicBlockAckTimeout ();
}

void
RegularWifiMac::SetCompressedBlockAckTimeout (Time blockAckTimeout)
{
  NS_LOG_FUNCTION (this << blockAckTimeout);
  m_low->SetCompressedBlockAckTimeout (blockAckTimeout);
}

Time
RegularWifiMac::GetCompressedBlockAckTimeout (void) const
{
  return m_low->GetCompressedBlockAckTimeout ();
}

void
RegularWifiMac::SetAddress (Mac48Address address)
{
  NS_LOG_FUNCTION (this << address);
  m_low->SetAddress (address);
}

Mac48Address
RegularWifiMac::GetAddress (void) const
{
  return m_low->GetAddress ();
}

void
RegularWifiMac::SetSsid (Ssid ssid)
{
  NS_LOG_FUNCTION (this << ssid);
  m_ssid = ssid;
}

Ssid
RegularWifiMac::GetSsid (void) const
{
  return m_ssid;
}

void
RegularWifiMac::SetBssid (Mac48Address bssid)
{
  NS_LOG_FUNCTION (this << bssid);
  m_low->SetBssid (bssid);
}

Mac48Address
RegularWifiMac::GetBssid (void) const
{
  return m_low->GetBssid ();
}

void
RegularWifiMac::SetPromisc (void)
{
  m_low->SetPromisc ();
}

void
RegularWifiMac::SetShortSlotTimeSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_shortSlotTimeSupported = enable;
}

bool
RegularWifiMac::GetShortSlotTimeSupported (void) const
{
  return m_shortSlotTimeSupported;
}

void
RegularWifiMac::SetRifsSupported (bool enable)
{
  NS_LOG_FUNCTION (this << enable);
  m_rifsSupported = enable;
}

bool
RegularWifiMac::GetRifsSupported (void) const
{
  return m_rifsSupported;
}

void
RegularWifiMac::Enqueue (Ptr<const Packet> packet,
                         Mac48Address to, Mac48Address from)
{
  //We expect RegularWifiMac subclasses which do support forwarding (e.g.,
  //AP) to override this method. Therefore, we throw a fatal error if
  //someone tries to invoke this method on a class which has not done
  //this.
  NS_FATAL_ERROR ("This MAC entity (" << this << ", " << GetAddress ()
                                      << ") does not support Enqueue() with from address");
}

bool
RegularWifiMac::SupportsSendFrom (void) const
{
  return false;
}

void
RegularWifiMac::ForwardUp (Ptr<Packet> packet, Mac48Address from, Mac48Address to)
{
  NS_LOG_FUNCTION (this << packet << from << to);
  m_forwardUp (packet, from, to);
}

void
RegularWifiMac::Receive (Ptr<Packet> packet, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << packet << hdr);

  Mac48Address to = hdr->GetAddr1 ();
  Mac48Address from = hdr->GetAddr2 ();

  //We don't know how to deal with any frame that is not addressed to
  //us (and odds are there is nothing sensible we could do anyway),
  //so we ignore such frames.
  //
  //The derived class may also do some such filtering, but it doesn't
  //hurt to have it here too as a backstop.
  if (to != GetAddress ())
    {
      return;
    }

  if (hdr->IsMgt () && hdr->IsAction ())
    {
      //There is currently only any reason for Management Action
      //frames to be flying about if we are a QoS STA.
      NS_ASSERT (m_qosSupported);

      WifiActionHeader actionHdr;
      packet->RemoveHeader (actionHdr);

      switch (actionHdr.GetCategory ())
        {
        case WifiActionHeader::BLOCK_ACK:

          switch (actionHdr.GetAction ().blockAck)
            {
            case WifiActionHeader::BLOCK_ACK_ADDBA_REQUEST:
              {
                MgtAddBaRequestHeader reqHdr;
                packet->RemoveHeader (reqHdr);

                //We've received an ADDBA Request. Our policy here is
                //to automatically accept it, so we get the ADDBA
                //Response on it's way immediately.
                SendAddBaResponse (&reqHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE:
              {
                MgtAddBaResponseHeader respHdr;
                packet->RemoveHeader (respHdr);

                //We've received an ADDBA Response. We assume that it
                //indicates success after an ADDBA Request we have
                //sent (we could, in principle, check this, but it
                //seems a waste given the level of the current model)
                //and act by locally establishing the agreement on
                //the appropriate queue.
                AcIndex ac = QosUtilsMapTidToAc (respHdr.GetTid ());
                m_edca[ac]->GotAddBaResponse (&respHdr, from);
                //This frame is now completely dealt with, so we're done.
                return;
              }
            case WifiActionHeader::BLOCK_ACK_DELBA:
              {
                MgtDelBaHeader delBaHdr;
                packet->RemoveHeader (delBaHdr);

                if (delBaHdr.IsByOriginator ())
                  {
                    //This DELBA frame was sent by the originator, so
                    //this means that an ingoing established
                    //agreement exists in MacLow and we need to
                    //destroy it.
                    m_low->DestroyBlockAckAgreement (from, delBaHdr.GetTid ());
                  }
                else
                  {
                    //We must have been the originator. We need to
                    //tell the correct queue that the agreement has
                    //been torn down
                    AcIndex ac = QosUtilsMapTidToAc (delBaHdr.GetTid ());
                    m_edca[ac]->GotDelBaFrame (&delBaHdr, from);
                  }
                //This frame is now completely dealt with, so we're done.
                return;
              }
            default:
              NS_FATAL_ERROR ("Unsupported Action field in Block Ack Action frame");
              return;
            }
        default:
          NS_FATAL_ERROR ("Unsupported Action frame received");
          return;
        }
    }
  NS_FATAL_ERROR ("Don't know how to handle frame (type=" << hdr->GetType ());
}

void
RegularWifiMac::DeaggregateAmsduAndForward (Ptr<Packet> aggregatedPacket, const WifiMacHeader *hdr)
{
  NS_LOG_FUNCTION (this << aggregatedPacket << hdr);
  MsduAggregator::DeaggregatedMsdus packets = MsduAggregator::Deaggregate (aggregatedPacket);
  for (MsduAggregator::DeaggregatedMsdusCI i = packets.begin ();
       i != packets.end (); ++i)
    {
      ForwardUp ((*i).first, (*i).second.GetSourceAddr (),
                 (*i).second.GetDestinationAddr ());
    }
}

void
RegularWifiMac::SendAddBaResponse (const MgtAddBaRequestHeader *reqHdr,
                                   Mac48Address originator)
{
  NS_LOG_FUNCTION (this);
  WifiMacHeader hdr;
  hdr.SetType (WIFI_MAC_MGT_ACTION);
  hdr.SetAddr1 (originator);
  hdr.SetAddr2 (GetAddress ());
  hdr.SetAddr3 (GetAddress ());
  hdr.SetDsNotFrom ();
  hdr.SetDsNotTo ();

  MgtAddBaResponseHeader respHdr;
  StatusCode code;
  code.SetSuccess ();
  respHdr.SetStatusCode (code);
  //Here a control about queues type?
  respHdr.SetAmsduSupport (reqHdr->IsAmsduSupported ());

  if (reqHdr->IsImmediateBlockAck ())
    {
      respHdr.SetImmediateBlockAck ();
    }
  else
    {
      respHdr.SetDelayedBlockAck ();
    }
  respHdr.SetTid (reqHdr->GetTid ());
  //For now there's not no control about limit of reception. We
  //assume that receiver has no limit on reception. However we assume
  //that a receiver sets a bufferSize in order to satisfy next
  //equation: (bufferSize + 1) % 16 = 0 So if a recipient is able to
  //buffer a packet, it should be also able to buffer all possible
  //packet's fragments. See section 7.3.1.14 in IEEE802.11e for more details.
  respHdr.SetBufferSize (1023);
  respHdr.SetTimeout (reqHdr->GetTimeout ());

  WifiActionHeader actionHdr;
  WifiActionHeader::ActionValue action;
  action.blockAck = WifiActionHeader::BLOCK_ACK_ADDBA_RESPONSE;
  actionHdr.SetAction (WifiActionHeader::BLOCK_ACK, action);

  Ptr<Packet> packet = Create<Packet> ();
  packet->AddHeader (respHdr);
  packet->AddHeader (actionHdr);

  //We need to notify our MacLow object as it will have to buffer all
  //correctly received packets for this Block Ack session
  m_low->CreateBlockAckAgreement (&respHdr, originator,
                                  reqHdr->GetStartingSequence ());

  //It is unclear which queue this frame should go into. For now we
  //bung it into the queue corresponding to the TID for which we are
  //establishing an agreement, and push it to the head.
  m_edca[QosUtilsMapTidToAc (reqHdr->GetTid ())]->PushFront (packet, hdr);
}

TypeId
RegularWifiMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RegularWifiMac")
    .SetParent<WifiMac> ()
    .SetGroupName ("Wifi")
    .AddAttribute ("QosSupported",
                   "This Boolean attribute is set to enable 802.11e/WMM-style QoS support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetQosSupported,
                                        &RegularWifiMac::GetQosSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("HtSupported",
                   "This Boolean attribute is set to enable 802.11n support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetHtSupported,
                                        &RegularWifiMac::GetHtSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("VhtSupported",
                   "This Boolean attribute is set to enable 802.11ac support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetVhtSupported,
                                        &RegularWifiMac::GetVhtSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("HeSupported",
                   "This Boolean attribute is set to enable 802.11ax support at this STA.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetHeSupported,
                                        &RegularWifiMac::GetHeSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("CtsToSelfSupported",
                   "Use CTS to Self when using a rate that is not in the basic rate set.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetCtsToSelfSupported),
                   MakeBooleanChecker ())
		//////////////////////////////////////////////////////////////////////////////////

   .AddAttribute ("DVO_MaxAmsduSize",
				                      "Maximum length in bytes of an A-MSDU for AC_VD access class. "
				                      "Value 0 means A-MSDU is disabled for that AC.",
				                      UintegerValue (0),
				                      MakeUintegerAccessor (&RegularWifiMac::SetDvoMaxAmsduSize),
				                      MakeUintegerChecker<uint16_t> (0, 11426))
    .AddAttribute ("VO_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VO access class. "
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11426))

   
   .AddAttribute ("KVI_MaxAmsduSize",
				 "Maximum length in bytes of an A-MSDU for AC_KVI access class. "
				 "Value 0 means A-MSDU is disabled for that AC.",
				  UintegerValue (0),
				  MakeUintegerAccessor (&RegularWifiMac::SetKviMaxAmsduSize),
				  MakeUintegerChecker<uint16_t> (0, 11426))
   .AddAttribute ("DVI_MaxAmsduSize",
				  "Maximum length in bytes of an A-MSDU for AC_DVI access class. "
				  "Value 0 means A-MSDU is disabled for that AC.",
				   UintegerValue (0),
				 				  MakeUintegerAccessor (&RegularWifiMac::SetDviMaxAmsduSize),
				 				  MakeUintegerChecker<uint16_t> (0, 11426))
    .AddAttribute ("VI_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_VI access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11426))
    .AddAttribute ("BE_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BE access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11426))
    .AddAttribute ("BK_MaxAmsduSize",
                   "Maximum length in bytes of an A-MSDU for AC_BK access class."
                   "Value 0 means A-MSDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkMaxAmsduSize),
                   MakeUintegerChecker<uint16_t> (0, 11426))

		////////////////////////////////////////////////////////////////////////////////////////

    .AddAttribute ("DVO_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_DVO access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetDvoMaxAmpduSize),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("VO_MaxAmpduSize",
                   "Maximum length in bytes of an A-MPDU for AC_VO access class."
                   "Value 0 means A-MPDU is disabled for that AC.",
                   UintegerValue (65535),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoMaxAmpduSize),
                   MakeUintegerChecker<uint16_t> ())
  
	 .AddAttribute ("KVI_MaxAmpduSize",
				    "Maximum length in bytes of an A-MPDU for AC_KVI access class."
				    "Value 0 means A-MPDU is disabled for that AC.",
				     UintegerValue (0),
				   MakeUintegerAccessor (&RegularWifiMac::SetKviMaxAmpduSize),
				   MakeUintegerChecker<uint16_t> ())
	   .AddAttribute ("DVI_MaxAmpduSize",
                     "Maximum length in bytes of an A-MPDU for AC_KVI access class."
                     "Value 0 means A-MPDU is disabled for that AC.",
                      UintegerValue (0),
				      MakeUintegerAccessor (&RegularWifiMac::SetDviMaxAmpduSize),
				      MakeUintegerChecker<uint16_t> ())
	   .AddAttribute ("VI_MaxAmpduSize",
				      "Maximum length in bytes of an A-MPDU for AC_VI access class."
				      "Value 0 means A-MPDU is disabled for that AC.",
				       UintegerValue (65535),
				       MakeUintegerAccessor (&RegularWifiMac::SetViMaxAmpduSize),
				       MakeUintegerChecker<uint16_t> ())
				       .AddAttribute ("BE_MaxAmpduSize",
				                      "Maximum length in bytes of an A-MPDU for AC_BE access class."
				                      "Value 0 means A-MPDU is disabled for that AC.",
				                      UintegerValue (65535),
				                      MakeUintegerAccessor (&RegularWifiMac::SetBeMaxAmpduSize),
				                      MakeUintegerChecker<uint16_t> ())
				       .AddAttribute ("BK_MaxAmpduSize",
				                      "Maximum length in bytes of an A-MPDU for AC_BK access class."
				                      "Value 0 means A-MPDU is disabled for that AC.",
				                      UintegerValue (0),
				                      MakeUintegerAccessor (&RegularWifiMac::SetBkMaxAmpduSize),
				                      MakeUintegerChecker<uint16_t> ())
				   //////////////////////////////////////////////////////////////////////////////
    .AddAttribute ("VO_BlockAckThreshold",
                   "If number of packets in VO queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("VI_BlockAckThreshold",
                   "If number of packets in VI queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BE_BlockAckThreshold",
                   "If number of packets in BE queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))
    .AddAttribute ("BK_BlockAckThreshold",
                   "If number of packets in BK queue reaches this value, "
                   "block ack mechanism is used. If this value is 0, block ack is never used."
                   "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckThreshold),
                   MakeUintegerChecker<uint8_t> (0, 64))

     .AddAttribute ("DVO_BlockAckThreshold",
				    "If number of packets in VD queue reaches this value, "
				     "block ack mechanism is used. If this value is 0, block ack is never used."
				     "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
				      UintegerValue (0),
				      MakeUintegerAccessor (&RegularWifiMac::SetDvoBlockAckThreshold),
				      MakeUintegerChecker<uint8_t> (0, 64))
				     
	    .AddAttribute ("KVI_BlockAckThreshold",
				        "If number of packets in VH queue reaches this value, "
				        "block ack mechanism is used. If this value is 0, block ack is never used."
				        "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
				         UintegerValue (0),
				         MakeUintegerAccessor (&RegularWifiMac::SetKviBlockAckThreshold),
				         MakeUintegerChecker<uint8_t> (0, 64))

	     .AddAttribute ("DVI_BlockAckThreshold",
	                     "If number of packets in VH queue reaches this value, "
				         "block ack mechanism is used. If this value is 0, block ack is never used."
					     "When A-MPDU is enabled, block ack mechanism is used regardless of this value.",
					      UintegerValue (0),
						  MakeUintegerAccessor (&RegularWifiMac::SetDviBlockAckThreshold),
					      MakeUintegerChecker<uint8_t> (0, 64))

			///////////////////////////////////////////////////////////////////////////////////////////////

    .AddAttribute ("VO_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_VO. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetVoBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("VI_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_VI. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetViBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BE_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_BE. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBeBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("BK_BlockAckInactivityTimeout",
                   "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
                   "inactivity for AC_BK. If this value isn't equal to 0 a timer start after that a"
                   "block ack setup is completed and will be reset every time that a block ack"
                   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&RegularWifiMac::SetBkBlockAckInactivityTimeout),
                   MakeUintegerChecker<uint16_t> ())


   .AddAttribute ("DVO_BlockAckInactivityTimeout",
			      "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
				   "inactivity for AC_VD. If this value isn't equal to 0 a timer start after that a"
				   "block ack setup is completed and will be reset every time that a block ack"
				   "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
				    UintegerValue (0),
				    MakeUintegerAccessor (&RegularWifiMac::SetDvoBlockAckInactivityTimeout),
				    MakeUintegerChecker<uint16_t> ())
				     
    .AddAttribute ("KVI_BlockAckInactivityTimeout",
			    "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
			    "inactivity for AC_BK. If this value isn't equal to 0 a timer start after that a"
			    "block ack setup is completed and will be reset every time that a block ack"
		       "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
		       UintegerValue (0),
		       MakeUintegerAccessor (&RegularWifiMac::SetKviBlockAckInactivityTimeout),
	           MakeUintegerChecker<uint16_t> ())

	 .AddAttribute ("DVI_BlockAckInactivityTimeout",
	            "Represents max time (blocks of 1024 micro seconds) allowed for block ack"
		       "inactivity for AC_BK. If this value isn't equal to 0 a timer start after that a"
	           "block ack setup is completed and will be reset every time that a block ack"
	           "frame is received. If this value is 0, block ack inactivity timeout won't be used.",
               UintegerValue (0),
				MakeUintegerAccessor (&RegularWifiMac::SetDviBlockAckInactivityTimeout),
		         MakeUintegerChecker<uint16_t> ())


				   ///////////////////////////////////////////////////////////////////////////////////////



    .AddAttribute ("ShortSlotTimeSupported",
                   "Whether or not short slot time is supported (only used by ERP APs or STAs).",
                   BooleanValue (true),
                   MakeBooleanAccessor (&RegularWifiMac::SetShortSlotTimeSupported,
                                        &RegularWifiMac::GetShortSlotTimeSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("RifsSupported",
                   "Whether or not RIFS is supported (only used by HT APs or STAs).",
                   BooleanValue (false),
                   MakeBooleanAccessor (&RegularWifiMac::SetRifsSupported,
                                        &RegularWifiMac::GetRifsSupported),
                   MakeBooleanChecker ())
    .AddAttribute ("Txop",
                   "The Txop object.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetTxop),
                   MakePointerChecker<Txop> ())
    .AddAttribute ("VO_Txop",
                   "Queue that manages packets belonging to AC_VO access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVOQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("VI_Txop",
                   "Queue that manages packets belonging to AC_VI access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetVIQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("BE_Txop",
                   "Queue that manages packets belonging to AC_BE access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBEQueue),
                   MakePointerChecker<QosTxop> ())
    .AddAttribute ("BK_Txop",
                   "Queue that manages packets belonging to AC_BK access class.",
                   PointerValue (),
                   MakePointerAccessor (&RegularWifiMac::GetBKQueue),
                   MakePointerChecker<QosTxop> ())
  .AddAttribute ("DVO_Txop",
			     "Queue that manages packets belonging to AC_DVO access class.",
				  PointerValue (),
			     MakePointerAccessor (&RegularWifiMac::GetDVOQueue),
				  MakePointerChecker<QosTxop> ())
				
    .AddAttribute ("KVI_Txop",
				    "Queue that manages packets belonging to AC_KVI access class.",
				     PointerValue (),
				     MakePointerAccessor (&RegularWifiMac::GetKVIQueue),
				     MakePointerChecker<QosTxop> ())
   .AddAttribute ("DVI_Txop",
			       "Queue that manages packets belonging to AC_DVI access class.",
		   	       PointerValue (),
	              MakePointerAccessor (&RegularWifiMac::GetDVIQueue),
			     MakePointerChecker<QosTxop> ())


    .AddTraceSource ("TxOkHeader",
                     "The header of successfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txOkCallback),
                     "ns3::WifiMacHeader::TracedCallback")
    .AddTraceSource ("TxErrHeader",
                     "The header of unsuccessfully transmitted packet.",
                     MakeTraceSourceAccessor (&RegularWifiMac::m_txErrCallback),
                     "ns3::WifiMacHeader::TracedCallback")
  ;
  return tid;
}

void
RegularWifiMac::FinishConfigureStandard (WifiPhyStandard standard)
{
  NS_LOG_FUNCTION (this << standard);
  uint32_t cwmin = 0;
  uint32_t cwmax = 0;
  switch (standard)
    {
    case WIFI_PHY_STANDARD_80211ax_5GHZ:
      SetHeSupported (true);
    case WIFI_PHY_STANDARD_80211ac:
      SetVhtSupported (true);
    case WIFI_PHY_STANDARD_80211n_5GHZ:
      SetHtSupported (true);
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_PHY_STANDARD_80211ax_2_4GHZ:
      SetHeSupported (true);
    case WIFI_PHY_STANDARD_80211n_2_4GHZ:
      SetHtSupported (true);
    case WIFI_PHY_STANDARD_80211g:
      SetErpSupported (true);
    case WIFI_PHY_STANDARD_holland:
    case WIFI_PHY_STANDARD_80211a:
    case WIFI_PHY_STANDARD_80211_10MHZ:
    case WIFI_PHY_STANDARD_80211_5MHZ:
      cwmin = 15;
      cwmax = 1023;
      break;
    case WIFI_PHY_STANDARD_80211b:
      SetDsssSupported (true);
      cwmin = 31;
      cwmax = 1023;
      break;
    default:
      NS_FATAL_ERROR ("Unsupported WifiPhyStandard in RegularWifiMac::FinishConfigureStandard ()");
    }

  ConfigureContentionWindow (cwmin, cwmax);
}

void
RegularWifiMac::ConfigureContentionWindow (uint32_t cwMin, uint32_t cwMax)
{
  bool isDsssOnly = m_dsssSupported && !m_erpSupported;
  //The special value of AC_BE_NQOS which exists in the Access
  //Category enumeration allows us to configure plain old DCF.
  ConfigureDcf (m_txop, cwMin, cwMax, isDsssOnly, AC_BE_NQOS);

  //Now we configure the EDCA functions
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      ConfigureDcf (i->second, cwMin, cwMax, isDsssOnly, i->first);
    }
}

void
RegularWifiMac::TxOk (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txOkCallback (hdr);
}

void
RegularWifiMac::TxFailed (const WifiMacHeader &hdr)
{
  NS_LOG_FUNCTION (this << hdr);
  m_txErrCallback (hdr);
}

void
RegularWifiMac::ConfigureAggregation (void)
{
  NS_LOG_FUNCTION (this);
  if (GetDVOQueue ()->GetMsduAggregator () != 0)
    {
      GetDVOQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_dvoMaxAmsduSize);
    }
  if (GetVOQueue ()->GetMsduAggregator () != 0)
    {
      GetVOQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_voMaxAmsduSize);
    }
 
   if (GetKVIQueue ()->GetMsduAggregator () != 0)
      {
        GetKVIQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_kviMaxAmsduSize);
      }
   if (GetDVIQueue ()->GetMsduAggregator () != 0)
      {
        GetDVIQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_dviMaxAmsduSize);
      }
    if (GetVIQueue ()->GetMsduAggregator () != 0)
      {
        GetVIQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_viMaxAmsduSize);
      }
  if (GetBEQueue ()->GetMsduAggregator () != 0)
    {
      GetBEQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_beMaxAmsduSize);
    }
  if (GetBKQueue ()->GetMsduAggregator () != 0)
    {
      GetBKQueue ()->GetMsduAggregator ()->SetMaxAmsduSize (m_bkMaxAmsduSize);
    }
  if (GetDVOQueue ()->GetMpduAggregator () != 0)
    {
      GetDVOQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_dvoMaxAmpduSize);
    }
  if (GetVOQueue ()->GetMpduAggregator () != 0)
    {
      GetVOQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_voMaxAmpduSize);
    }
  
    if (GetKVIQueue ()->GetMpduAggregator () != 0)
         {
           GetKVIQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_kviMaxAmpduSize);
         }
    if (GetDVIQueue ()->GetMpduAggregator () != 0)
         {
           GetDVIQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_dviMaxAmpduSize);
         }
       if (GetVIQueue ()->GetMpduAggregator () != 0)
         {
           GetVIQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_viMaxAmpduSize);
         }
  if (GetBEQueue ()->GetMpduAggregator () != 0)
    {
      GetBEQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_beMaxAmpduSize);
    }
  if (GetBKQueue ()->GetMpduAggregator () != 0)
    {
      GetBKQueue ()->GetMpduAggregator ()->SetMaxAmpduSize (m_bkMaxAmpduSize);
    }
}

void
RegularWifiMac::EnableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      if (i->second->GetMsduAggregator () == 0)
        {
          Ptr<MsduAggregator> msduAggregator = CreateObject<MsduAggregator> ();
          i->second->SetMsduAggregator (msduAggregator);
        }
      if (i->second->GetMpduAggregator () == 0)
        {
          Ptr<MpduAggregator> mpduAggregator = CreateObject<MpduAggregator> ();
          i->second->SetMpduAggregator (mpduAggregator);
        }
    }
  ConfigureAggregation ();
}

void
RegularWifiMac::DisableAggregation (void)
{
  NS_LOG_FUNCTION (this);
  for (EdcaQueues::const_iterator i = m_edca.begin (); i != m_edca.end (); ++i)
    {
      i->second->SetMsduAggregator (0);
      i->second->SetMpduAggregator (0);
    }
}

} //namespace ns3

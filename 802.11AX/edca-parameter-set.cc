/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2016 Sébastien Deronne
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
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "edca-parameter-set.h"
#include <cmath>

namespace ns3 {

EdcaParameterSet::EdcaParameterSet ()
  : m_qosInfo (0),
    m_reserved (0),
    m_acBE (0),
    m_acBK (0),
    m_acVI (0),  //m_acAVI // K Video
    m_acDVI (0),   // HD,Full HD Video
    m_acKVI (0),
    m_acVO (0), 
    m_acDVO (0),
    m_qosSupported (0)
{
}

WifiInformationElementId
EdcaParameterSet::ElementId () const
{
  return IE_EDCA_PARAMETER_SET;
}

void
EdcaParameterSet::SetQosSupported (uint8_t qosSupported)
{
  m_qosSupported = qosSupported;
}

uint8_t
EdcaParameterSet::IsQosSupported (void) const
{
  return ((m_acBE != 0) || (m_acBK != 0) ||(m_acVI != 0)||(m_acDVI != 0) || (m_acKVI != 0) || (m_acVO != 0) ||(m_acDVO != 0));
}

void
EdcaParameterSet::SetQosInfo (uint8_t qosInfo)
{
  m_qosInfo = qosInfo;
}

void
EdcaParameterSet::SetBeAifsn (uint8_t aifsn)
{
  m_acBE |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetBeAci (uint8_t aci)
{
  m_acBE |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetBeCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acBE |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetBeCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acBE |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetBeTxopLimit (uint16_t txop)
{
  m_acBE |= txop << 16;
}
/////////////////////////////////////////////////// setbe done

void
EdcaParameterSet::SetBkAifsn (uint8_t aifsn)
{
  m_acBK |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetBkAci (uint8_t aci)
{
  m_acBK |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetBkCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acBK |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetBkCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acBK |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetBkTxopLimit (uint16_t txop)
{
  m_acBK |= txop << 16;
}
/////////////////////////////////////////////////////////////////////// setbk done



void
EdcaParameterSet::SetViAifsn (uint8_t aifsn)
{
  m_acVI |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetViAci (uint8_t aci)
{
  m_acVI |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetViCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acVI |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetViCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acVI |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetViTxopLimit (uint16_t txop)
{
  m_acVI |= txop << 16;
}

//////////////////////////////////////////////////////////////////// set vi done


void
EdcaParameterSet::SetDviAifsn (uint8_t aifsn)
{
  m_acDVI |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetDviAci (uint8_t aci)
{
  m_acDVI |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetDviCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acDVI |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetDviCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acDVI |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetDviTxopLimit (uint16_t txop)
{
  m_acDVI |= txop << 16;
}

////////////////////////////////////////////////////////////////// set DVI done


void
EdcaParameterSet::SetKviAifsn (uint8_t aifsn)
{
  m_acKVI |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetKviAci (uint8_t aci)
{
  m_acKVI |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetKviCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acKVI |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetKviCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acKVI |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetKviTxopLimit (uint16_t txop)
{
  m_acKVI |= txop << 16;
}
/////////////////////////////////////////////////////////////// Set KVI Done

void
EdcaParameterSet::SetVoAifsn (uint8_t aifsn)
{
  m_acVO |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetVoAci (uint8_t aci)
{
  m_acVO |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetVoCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acVO |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetVoCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acVO |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetVoTxopLimit (uint16_t txop)
{
  m_acVO |= txop << 16;
}
//////////////////////////////////////////////////////////////// set vo done
void
EdcaParameterSet::SetDvoAifsn (uint8_t aifsn)
{
  m_acDVO |= (aifsn & 0x0f);
}

void
EdcaParameterSet::SetDvoAci (uint8_t aci)
{
  m_acDVO |= (aci & 0x03) << 5;
}

void
EdcaParameterSet::SetDvoCWmin (uint32_t cwMin)
{
  uint8_t ECWmin = static_cast<uint8_t> (log2 (cwMin + 1));
  m_acDVO |= (ECWmin & 0x0f) << 8;
}

void
EdcaParameterSet::SetDvoCWmax (uint32_t cwMax)
{
  uint8_t ECWmax = static_cast<uint8_t> (log2 (cwMax + 1));
  m_acDVO |= (ECWmax & 0x0f) << 12;
}

void
EdcaParameterSet::SetDvoTxopLimit (uint16_t txop)
{
  m_acDVO |= txop << 16;
}

////////////////////////////////////////////////////////////////  set DVO done


uint8_t
EdcaParameterSet::GetQosInfo (void) const
{
  return m_qosInfo;
}

//////////////////////////////////////////////////////////////////
uint8_t
EdcaParameterSet::GetBeAifsn (void) const
{
  return (m_acBE & 0x0f);
}

uint32_t
EdcaParameterSet::GetBeCWmin (void) const
{
  uint8_t ECWmin = ((m_acBE >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetBeCWmax (void) const
{
  uint8_t ECWmax = ((m_acBE >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetBeTxopLimit (void) const
{
  return (m_acBE >> 16);
}
////////////////////////////////////////////////////////////////////////////////  get be done
uint8_t
EdcaParameterSet::GetBkAifsn (void) const
{
  return (m_acBK & 0x0f);
}

uint32_t
EdcaParameterSet::GetBkCWmin (void) const
{
  uint8_t ECWmin = ((m_acBK >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetBkCWmax (void) const
{
  uint8_t ECWmax = ((m_acBK >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetBkTxopLimit (void) const
{
  return (m_acBK >> 16);
}
/////////////////////////////////////////////////////////////////////////////////// get bk done
uint8_t
EdcaParameterSet::GetViAifsn (void) const
{
  return (m_acVI & 0x0f);
}

uint32_t
EdcaParameterSet::GetViCWmin (void) const
{
  uint8_t ECWmin = ((m_acVI >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetViCWmax (void) const
{
  uint8_t ECWmax = ((m_acVI >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetViTxopLimit (void) const
{
  return (m_acVI >> 16);
}
////////////////////////////////////////////////////////////////////////////////  get vi done
uint8_t
EdcaParameterSet::GetDviAifsn (void) const
{
  return (m_acDVI & 0x0f);
}

uint32_t
EdcaParameterSet::GetDviCWmin (void) const
{
  uint8_t ECWmin = ((m_acDVI >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetDviCWmax (void) const
{
  uint8_t ECWmax = ((m_acDVI >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetDviTxopLimit (void) const
{
  return (m_acDVI >> 16);
}
///////////////////////////////////////////////////////////////////////////////   get DVI done
uint8_t
EdcaParameterSet::GetKviAifsn (void) const
{
  return (m_acKVI & 0x0f);
}

uint32_t
EdcaParameterSet::GetKviCWmin (void) const
{
  uint8_t ECWmin = ((m_acKVI >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetKviCWmax (void) const
{
  uint8_t ECWmax = ((m_acKVI >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetKviTxopLimit (void) const
{
  return (m_acKVI >> 16);
}

//////////////////////////////////////////////////////////////////////////  get KVI done.



uint8_t
EdcaParameterSet::GetVoAifsn (void) const
{
  return (m_acVO & 0x0f);
}

uint32_t
EdcaParameterSet::GetVoCWmin (void) const
{
  uint8_t ECWmin = ((m_acVO >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetVoCWmax (void) const
{
  uint8_t ECWmax = ((m_acVO >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetVoTxopLimit (void) const
{
  return (m_acVO >> 16);
}

//////////////////////////////////////////////////////////////  get vo done

uint8_t
EdcaParameterSet::GetDvoAifsn (void) const
{
  return (m_acDVO & 0x0f);
}

uint32_t
EdcaParameterSet::GetDvoCWmin (void) const
{
  uint8_t ECWmin = ((m_acDVO >> 8) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmin) - 1);
}

uint32_t
EdcaParameterSet::GetDvoCWmax (void) const
{
  uint8_t ECWmax = ((m_acDVO >> 12) & 0x0f);
  return static_cast<uint32_t> (exp2 (ECWmax) - 1);
}

uint16_t
EdcaParameterSet::GetDvoTxopLimit (void) const
{
  return (m_acDVO >> 16);
}
/////////////////////////////////////////////////////////// get DVO done

uint8_t
EdcaParameterSet::GetInformationFieldSize () const
{
  NS_ASSERT (m_qosSupported);
  return 18;
}



Buffer::Iterator
EdcaParameterSet::Serialize (Buffer::Iterator i) const
{
  if (!m_qosSupported)
    {
      return i;
    }
  return WifiInformationElement::Serialize (i);
}

uint16_t
EdcaParameterSet::GetSerializedSize () const
{
  if (!m_qosSupported)
    {
      return 0;
    }
  return WifiInformationElement::GetSerializedSize ();
}

void
EdcaParameterSet::SerializeInformationField (Buffer::Iterator start) const
{
  if (m_qosSupported)
    {
      start.WriteU8 (GetQosInfo ());
      start.WriteU8 (m_reserved);
      start.WriteU32 (m_acBE);
      start.WriteU32 (m_acBK);
      start.WriteU32 (m_acVI);
      start.WriteU32 (m_acDVI);
      start.WriteU32 (m_acKVI);
      start.WriteU32 (m_acVO);
      start.WriteU32 (m_acDVO);
    }
}

uint8_t
EdcaParameterSet::DeserializeInformationField (Buffer::Iterator start, uint8_t length)
{
  Buffer::Iterator i = start;
  m_qosInfo = i.ReadU8 ();
  m_reserved = i.ReadU8 ();
  m_acBE = i.ReadU32 ();
  m_acBK = i.ReadU32 ();
  m_acVI = i.ReadU32 ();
  m_acDVI = i.ReadU32 ();
  m_acKVI = i.ReadU32 ();
  m_acVO = i.ReadU32 ();
  m_acDVO = i.ReadU32 ();
  return length;
}

} //namespace ns3

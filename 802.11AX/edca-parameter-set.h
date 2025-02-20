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

#ifndef EDCA_PARAMETER_SET_H
#define EDCA_PARAMETER_SET_H

#include "wifi-information-element.h"

namespace ns3 {

/**
 * \brief The EDCA Parameter Set
 * \ingroup wifi
 *
 * This class knows how to serialise and deserialise the EDCA Parameter Set.
 */
class EdcaParameterSet : public WifiInformationElement
{
public:
  EdcaParameterSet ();
  /**
   * Set QOS supported function
   * \param qosSupported the QOS supported indicator
   */
  void SetQosSupported (uint8_t qosSupported);
  /**
   * Is QOS supported function
   * \returns the is QOS supported indicator
   */
  uint8_t IsQosSupported (void) const;

  /**
   * Set the QoS Info field in the EdcaParameterSet information element.
   *
   * \param qosInfo the QoS Info field in the EdcaParameterSet information element
   */
  void SetQosInfo (uint8_t qosInfo);
  /**
   * Set the AC_BE AIFSN field in the EdcaParameterSet information element.
   *
   * \param aifsn the AC_BE AIFSN field in the EdcaParameterSet information element
   */
  void SetBeAifsn (uint8_t aifsn);
  /**
   * Set the AC_BE ACI field in the EdcaParameterSet information element.
   *
   * \param aci the AC_BE ACI field in the EdcaParameterSet information element
   */
  void SetBeAci (uint8_t aci);
  /**
   * Set the AC_BE CWmin field in the EdcaParameterSet information element.
   *
   * \param cwMin the AC_BE CWmin field in the EdcaParameterSet information element
   */
  void SetBeCWmin (uint32_t cwMin);
  /**
   * Set the AC_BE CWmax field in the EdcaParameterSet information element.
   *
   * \param cwMax the AC_BE CWmax field in the EdcaParameterSet information element
   */
  void SetBeCWmax (uint32_t cwMax);
  /**
   * Set the AC_BE TXOP Limit field in the EdcaParameterSet information element.
   *
   * \param txop the AC_BE TXOP Limit field in the EdcaParameterSet information element
   */
  void SetBeTxopLimit (uint16_t txop);
  /**
   * Set the AC_BK AIFSN field in the EdcaParameterSet information element.
   *
   * \param aifsn the AC_BB AIFSN field in the EdcaParameterSet information element
   */
  void SetBkAifsn (uint8_t aifsn);
  /**
   * Set the AC_BK ACI field in the EdcaParameterSet information element.
   *
   * \param aci the AC_BK ACI field in the EdcaParameterSet information element
   */
  void SetBkAci (uint8_t aci);
  /**
   * Set the AC_BK CWmin field in the EdcaParameterSet information element.
   *
   * \param cwMin the AC_BK CWmin field in the EdcaParameterSet information element
   */
  void SetBkCWmin (uint32_t cwMin);
  /**
   * Set the AC_BK CWmax field in the EdcaParameterSet information element.
   *
   * \param cwMax the AC_BK CWmax field in the EdcaParameterSet information element
   */
  void SetBkCWmax (uint32_t cwMax);
  /**
   * Set the AC_BK TXOP Limit field in the EdcaParameterSet information element.
   *
   * \param txop the AC_BK TXOP Limit field in the EdcaParameterSet information element
   */
  void SetBkTxopLimit (uint16_t txop);
  /**
   * Set the AC_VI AIFSN field in the EdcaParameterSet information element.
   *
   * \param aifsn the AC_VI AIFSN field in the EdcaParameterSet information element
   */
  void SetViAifsn (uint8_t aifsn);
  /**
   * Set the AC_VI ACI field in the EdcaParameterSet information element.
   *
   * \param aci the AC_VI ACI field in the EdcaParameterSet information element
   */
  void SetViAci (uint8_t aci);
  /**
   * Set the AC_VI CWmin field in the EdcaParameterSet information element.
   *
   * \param cwMin the AC_VI CWmin field in the EdcaParameterSet information element
   */
  void SetViCWmin (uint32_t cwMin);
  /**
   * Set the AC_VI CWmax field in the EdcaParameterSet information element.
   *
   * \param cwMax the AC_VI CWmax field in the EdcaParameterSet information element
   */
  void SetViCWmax (uint32_t cwMax);
  /**
   * Set the AC_VI TXOP Limit field in the EdcaParameterSet information element.
   *
   * \param txop the AC_VI TXOP Limit field in the EdcaParameterSet information element
   */
  void SetViTxopLimit (uint16_t txop);
   ////////////////////////////////////////////////////////
  void SetDviAifsn (uint8_t aifsn);
  
  void SetDviAci (uint8_t aci);

  void SetDviCWmin (uint32_t cwMin);

  void SetDviCWmax (uint32_t cwMax);

  void SetDviTxopLimit (uint16_t txop);
  
  void SetKviAifsn (uint8_t aifsn);

    void SetKviAci (uint8_t aci);

    void SetKviCWmin (uint32_t cwMin);

    void SetKviCWmax (uint32_t cwMax);

    void SetKviTxopLimit (uint16_t txop);


 
 
  /////////////////////////////////////////////////////////

 /**
   * Set the AC_VO AIFSN field in the EdcaParameterSet information element.
   *
   * \param aifsn the AC_VO AIFSN field in the EdcaParameterSet information element
   */
  void SetVoAifsn (uint8_t aifsn);
  /**
   * Set the AC_VO ACI field in the EdcaParameterSet information element.
   *
   * \param aci the AC_VO ACI field in the EdcaParameterSet information element
   */
  void SetVoAci (uint8_t aci);
  /**
   * Set the AC_VO CWmin field in the EdcaParameterSet information element.
   *
   * \param cwMin the AC_VO CWmin field in the EdcaParameterSet information element
   */
  void SetVoCWmin (uint32_t cwMin);
  /**
   * Set the AC_VO CWmax field in the EdcaParameterSet information element.
   *
   * \param cwMax the AC_VO CWmax field in the EdcaParameterSet information element
   */
  void SetVoCWmax (uint32_t cwMax);
  /**
   * Set the AC_VO TXOP Limit field in the EdcaParameterSet information element.
   *
   * \param txop the AC_VO TXOP Limit field in the EdcaParameterSet information element
   */
  void SetVoTxopLimit (uint16_t txop);
   ////////////////////////////////////////////////////////

    void SetDvoAifsn (uint8_t aifsn);
  
   void SetDvoAci (uint8_t aci);

   void SetDvoCWmin (uint32_t cwMin);

   void SetDvoCWmax (uint32_t cwMax);
  
   void SetDvoTxopLimit (uint16_t txop);
  //////////////////////////////////////////////////////////
  /**
   * Return the QoS Info field in the EdcaParameterSet information element.
   *
   * \return the QoS Info field in the EdcaParameterSet information element
   */
  uint8_t GetQosInfo (void) const;
  /**
   * Return the AC_BE AIFSN field in the EdcaParameterSet information element.
   *
   * \return the AC_BE AIFSN field in the EdcaParameterSet information element
   */
  uint8_t GetBeAifsn (void) const;
  /**
   * Return the AC_BE CWmin field in the EdcaParameterSet information element.
   *
   * \return the AC_BE CWmin field in the EdcaParameterSet information element
   */
  uint32_t GetBeCWmin (void) const;
  /**
   * Return the AC_BE CWmax field in the EdcaParameterSet information element.
   *
   * \return the AC_BE CWmax field in the EdcaParameterSet information element
   */
  uint32_t GetBeCWmax (void) const;
  /**
   * Return the AC_BE TXOP Limit field in the EdcaParameterSet information element.
   *
   * \return the AC_BE TXOP Limit field in the EdcaParameterSet information element
   */
  uint16_t GetBeTxopLimit (void) const;
  /**
   * Return the AC_BK AIFSN field in the EdcaParameterSet information element.
   *
   * \return the AC_BK AIFSN field in the EdcaParameterSet information element
   */
  uint8_t GetBkAifsn (void) const;
  /**
   * Return the AC_BK CWmin field in the EdcaParameterSet information element.
   *
   * \return the AC_BK CWmin field in the EdcaParameterSet information element
   */
  uint32_t GetBkCWmin (void) const;
  /**
   * Return the AC_BK CWmax field in the EdcaParameterSet information element.
   *
   * \return the AC_BK CWmax field in the EdcaParameterSet information element
   */
  uint32_t GetBkCWmax (void) const;
  /**
   * Return the AC_BK TXOP Limit field in the EdcaParameterSet information element.
   *
   * \return the AC_BK TXOP Limit field in the EdcaParameterSet information element
   */
  uint16_t GetBkTxopLimit (void) const;
  /**
   * Return the AC_VI AIFSN field in the EdcaParameterSet information element.
   *
   * \return the AC_VI AIFSN field in the EdcaParameterSet information element
   */
  uint8_t GetViAifsn (void) const;
  /**
   * Return the AC_VI CWmin field in the EdcaParameterSet information element.
   *
   * \return the AC_VI CWmin field in the EdcaParameterSet information element
   */
  uint32_t GetViCWmin (void) const;
  /**
   * Return the AC_VI CWmax field in the EdcaParameterSet information element.
   *
   * \return the AC_VI CWmax field in the EdcaParameterSet information element
   */
  uint32_t GetViCWmax (void) const;
  /**
   * Return the AC_VI TXOP Limit field in the EdcaParameterSet information element.
   *
   * \return the AC_VI TXOP Limit field in the EdcaParameterSet information element
   */
  uint16_t GetViTxopLimit (void) const;

///////////////////////////////////////////////////////////////////////////////////////////


  uint8_t GetDviAifsn (void) const;
  
  uint32_t GetDviCWmin (void) const;
 
  uint32_t GetDviCWmax (void) const;
 
  uint16_t GetDviTxopLimit (void) const;

  uint8_t GetKviAifsn (void) const;

    uint32_t GetKviCWmin (void) const;

    uint32_t GetKviCWmax (void) const;

    uint16_t GetKviTxopLimit (void) const;






//////////////////////////////////////////////////////////////////////////////////////////
  /**
   * Return the AC_VO AIFSN field in the EdcaParameterSet information element.
   *
   * \return the AC_VO AIFSN field in the EdcaParameterSet information element
   */
  uint8_t GetVoAifsn (void) const;
  /**
   * Return the AC_VO CWmin field in the EdcaParameterSet information element.
   *
   * \return the AC_VO CWmin field in the EdcaParameterSet information element
   */
  uint32_t GetVoCWmin (void) const;
  /**
   * Return the AC_VO CWmax field in the EdcaParameterSet information element.
   *
   * \return the AC_VO CWmax field in the EdcaParameterSet information element
   */
  uint32_t GetVoCWmax (void) const;
  /**
   * Return the AC_VO TXOP Limit field in the EdcaParameterSet information element.
   *
   * \return the AC_VO TXOP Limit field in the EdcaParameterSet information element
   */
  uint16_t GetVoTxopLimit (void) const;

////////////////////////////////////////////////

   uint8_t GetDvoAifsn (void) const;
  
   uint32_t GetDvoCWmin (void) const;
 
   uint32_t GetDvoCWmax (void) const;
 
   uint16_t GetDvoTxopLimit (void) const;

////////////////////////////////////////////////////

  /**
   * Element ID function
   * \returns the wifi information element ID
   */
  WifiInformationElementId ElementId () const;
  /**
   * Get information field size function
   * \returns the information field size
   */
  uint8_t GetInformationFieldSize () const;
  /**
   * Serialize information field function
   * \param start the iterator
   */
  void SerializeInformationField (Buffer::Iterator start) const;
  /**
   * Deserialize information field function
   * \param start the iterator
   * \param length the length
   * \returns the size
   */
  uint8_t DeserializeInformationField (Buffer::Iterator start, uint8_t length);

  /**
   * This information element is a bit special in that it is only
   * included if the STA is a QoS STA. To support this we
   * override the Serialize and GetSerializedSize methods of
   * WifiInformationElement.
   *
   * \param start
   *
   * \return an iterator
   */
  Buffer::Iterator Serialize (Buffer::Iterator start) const;
  /**
   * Return the serialized size of this EDCA Parameter Set.
   *
   * \return the serialized size of this EDCA Parameter Set
   */
  uint16_t GetSerializedSize () const;


private:
  uint8_t m_qosInfo;  ///< QOS info
  uint8_t m_reserved; ///< reserved
  uint32_t m_acBE;    ///< AC_BE
  uint32_t m_acBK;    ///< AC_BK                  ///< AC_VI
  uint32_t m_acVI;
  uint32_t m_acDVI;
  uint32_t m_acKVI;
  uint32_t m_acVO;    ///< AC_VO
  uint32_t m_acDVO;

  /// This is used to decide whether this element should be added to the frame or not
  bool m_qosSupported;
};

} //namespace ns3

#endif /* EDCA_PARAMETER_SET_H */

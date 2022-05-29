/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013-2021 Regents of the University of California.
 *                         Harbin Institute of Technology
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 *
 * @author Zhongda Xia <xiazhongda@hit.edu.cn>
 */

#ifndef NDN_KITE_ACK_HPP
#define NDN_KITE_ACK_HPP

#include "ndn-cxx/prefix-announcement.hpp"

namespace ndn {
namespace kite {

/** \brief Use kite::Ack to encode and decode KITE acknowledgments.
 *  \sa https://redmine.named-data.net/projects/ndn-tlv/wiki/KITE#KITE-Acknowledgment
 */
class Ack
{
public:
  class Error : public tlv::Error
  {
  public:
    using tlv::Error::Error;
  };

  /** \brief Construct an empty KITE acknowledgment.
   *  \post getPrefixAnnouncement() == nullopt
   */
  Ack();

  /** \brief Decode a KITE acknowledgment from Data.
   *  \throw Error \p data is not a KITE acknowledgment.
   *
   *  This will not check whether the encoded prefix announcement matches the prefix specified
   *  in the name. A forwarder should do the check against the corresponding Request afterwards
   *  using kite::Request::canMatch().
   */
  explicit
  Ack(const Data& data);

  /** \brief Create a KITE acknowledgment as a response to a KITE request.
   *  \param interest a KITE request.
   *  \param keyChain KeyChain to sign the KITE acknowledgment.
   *  \param si signing parameters.
   *
   *  \throw std::invalid_argument member prefix announcement not set, or the prefix specified in
   *                               the member prefix announcement is not the same as the producer prefix
   *                               specified in the name of \p interest
   */
  Data
  makeData(const Interest& interest, KeyChain& keyChain, const security::SigningInfo& si = security::SigningInfo()) const;

  /** \brief Get the prefix announcement.
   */
  const optional<PrefixAnnouncement>&
  getPrefixAnnouncement() const
  {
    return m_prefixAnn;
  }

  /** \brief Set the prefix announcement.
   *  \param prefixAnn the prefix announcement to encode into this KITE acknowledgment,
   *                   if it is not signed, it will be signed later in makeData().
   */
  Ack&
  setPrefixAnnouncement(PrefixAnnouncement prefixAnn);

private:
  optional<PrefixAnnouncement> m_prefixAnn;
};

} // namespace kite
} // namespace ndn

#endif // NDN_KITE_ACK_HPP

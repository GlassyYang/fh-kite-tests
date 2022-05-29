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

#include "ndn-cxx/kite/ack.hpp"
#include "ndn-cxx/kite/request.hpp"

namespace ndn {
namespace kite {

Ack::Ack() = default;

Ack::Ack(const Data& data)
{
  if (data.getContentType() != tlv::ContentType_KiteAck) {
    NDN_THROW(Error("Not a KITE acknowledgment, ContentType is " +
                                    to_string(data.getContentType())));
  }

  const Block& payload = data.getContent();
  payload.parse();

  try {
    m_prefixAnn.emplace(Data(payload.get(tlv::Data)));
  }
  catch (tlv::Error) {
    NDN_THROW_NESTED(Error("No valid prefix announcement"));
  }
}

Data
Ack::makeData(const Interest& interest, KeyChain& keyChain, const security::SigningInfo& si) const
{
  if (!m_prefixAnn) {
    NDN_THROW(std::invalid_argument("Prefix announcement not set"));
  }

  auto extractedPrefixes = extractPrefixes(interest.getName());
  if (extractedPrefixes.second != m_prefixAnn->getAnnouncedName()) {
    NDN_THROW(std::invalid_argument("Producer prefix mismatch"));
  }

  Data data(interest.getName());
  data.setContentType(tlv::ContentType_KiteAck);

  Block content(tlv::Content);
  content.push_back(m_prefixAnn->toData(keyChain, si).wireEncode());
  content.encode();
  data.setContent(content);

  keyChain.sign(data, si);

  return data;
}

Ack&
Ack::setPrefixAnnouncement(PrefixAnnouncement prefixAnn)
{
  m_prefixAnn = prefixAnn;
  return *this;
}

} // namespace kite
} // namespace ndn

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

#include "ndn-cxx/kite/request.hpp"
#include "ndn-cxx/encoding/tlv-nfd.hpp"

namespace ndn {
namespace kite {

std::pair<Name, Name>
extractPrefixes(const Name& name)
{
  auto it = std::find(name.begin(), name.end(), KITE_KEYWORD);
  if (it == name.begin()) {
    NDN_THROW(std::invalid_argument("Missing RV prefix"));
  }

  if (it == name.end()) {
    NDN_THROW(std::invalid_argument("Missing \"KITE\" keyword"));
  }

  size_t keywordPos = it - name.begin();

  Name rvPrefix(name.getPrefix(keywordPos));

  Name producerSuffix;
  // exclude RV prefix, keyword and param digest
  int producerSuffixLength = name.size() - keywordPos - 1 - 1;
  if (producerSuffixLength > 0)
    producerSuffix = name.getSubName(keywordPos + 1, producerSuffixLength);
  else
    NDN_THROW(std::invalid_argument("Missing producer suffix"));

  return {rvPrefix, producerSuffix};
}

Request::Request() = default;

void
Request::decode(const Interest& interest)
{
  if (!interest.isSigned())
    NDN_THROW(Error("Not signed"));
  auto si = interest.getSignatureInfo();
  if (!si->getNonce() || !si->getTime())
    NDN_THROW(Error("Missing nonce or timestamp"));

  try {
    std::tie(m_rvPrefix, m_producerSuffix) = extractPrefixes(interest.getName());
  }
  catch (const std::invalid_argument&) {
    NDN_THROW(Error("Name is in the wrong format"));
  }

  m_nonce = *si->getNonce();
  m_timestamp = *si->getTime();

  BOOST_ASSERT(interest.hasApplicationParameters());
  Block params = interest.getApplicationParameters();
  params.parse();
  auto element = params.find(tlv::nfd::ExpirationPeriod);
  if (element != params.elements_end()) {
    Block expirationBlock = *element;
    m_expiration = time::milliseconds(readNonNegativeInteger(expirationBlock));
  }
}

Interest
Request::makeInterest(security::InterestSigner& signer, const security::SigningInfo& si)
{
  if (m_rvPrefix.empty())
    NDN_THROW(std::invalid_argument("RV prefix not set"));
  if (m_producerSuffix.empty())
    NDN_THROW(std::invalid_argument("Producer suffix not set"));

  Interest interest(Name(m_rvPrefix).append(KITE_KEYWORD).append(m_producerSuffix));
  interest.setCanBePrefix(false);
  if (m_expiration) {
    Block parameters = makeNonNegativeIntegerBlock(tlv::nfd::ExpirationPeriod, m_expiration->count());
    interest.setApplicationParameters(parameters);
  }

  /*
  if (si.getSignedInterestFormat() != security::SignedInterestFormat::V03)
    NDN_THROW(std::invalid_argument("Signed Interest format is not v0.3"));
  */
  signer.makeSignedInterest(interest, si,
                            security::InterestSigner::WantNonce | security::InterestSigner::WantTime);
  // update members
  auto preparedSi = interest.getSignatureInfo();
  m_nonce.emplace(*preparedSi->getNonce());
  m_timestamp = *preparedSi->getTime();
  return interest;
}

Request&
Request::setRvPrefix(Name rvPrefix)
{
  m_rvPrefix = std::move(rvPrefix);
  return *this;
}

Request&
Request::setProducerSuffix(Name producerSuffix)
{
  m_producerSuffix = std::move(producerSuffix);
  return *this;
}

Request&
Request::setExpiration(time::milliseconds expiration)
{
  if (expiration < 0_ms) {
    NDN_THROW(std::invalid_argument("Negative expiration period"));
  }
  m_expiration = expiration;
  return *this;
}

} // namespace kite
} // namespace ndn

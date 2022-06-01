/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2015-2019, Harbin Institute of Technology.
 *
 * This file is part of ndn-tools (Named Data Networking Essential Tools).
 * See AUTHORS.md for complete list of ndn-tools authors and contributors.
 *
 * ndn-tools is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndn-tools is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndn-tools, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @author Zhongda Xia <xiazhongda@hit.edu.cn>
 */

#include "producer.hpp"

#include <ndn-cxx/kite/ack.hpp>
#include <ndn-cxx/kite/request.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace ndn {
namespace kite {
namespace producer {

NDN_LOG_INIT(kite.producer);

Producer::Producer(Face& face, KeyChain& keyChain, const Options& options)
  : m_options(options)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_scheduler(m_face.getIoService())
{
}

void
Producer::start()
{
  // only the first prefix now
  Name producerPrefix(m_options.prefixPairs[0].producerSuffix);
  m_registeredPrefix = m_face.setInterestFilter(
                       producerPrefix,
                       bind(&Producer::onInterest, this, _1, _2),
                       [] (const auto&, const auto& reason) {
                         NDN_THROW(std::runtime_error("Failed to register prefix: " + reason));
                       });
  
  // sendKiteRequest();
}

void
Producer::stop()
{
  m_registeredPrefix.cancel();
  m_scheduler.cancelAllEvents();
}

void
Producer::onInterest(const InterestFilter& filter, const Interest& interest)
{
  afterReceive(interest.getName());

  NDN_LOG_DEBUG("Received consumer Interest: " << interest.getName()
                << ", under filter: " << filter.getPrefix());
  switch(m_options.action) {
    case 0:
    {
      Name producerPrefix(filter.getPrefix());
      auto data = make_shared<Data>(interest.getName());
      data->setContent(interest.wireEncode());
      m_keyChain.sign(*data, ndn::security::signingByIdentity(producerPrefix));
      m_face.put(*data);
      break;
    }
    case 1:
    {
      auto nack = make_shared<lp::Nack>(interest);
      m_face.put(*nack);
      break;
    }
  }
}

void
Producer::onData(const Interest& interest, const Data& data)
{
  Request req;
  req.decode(interest);
  Ack ack(data);

  NDN_LOG_DEBUG("Received Ack for: " << req.getProducerPrefix());
}

void
Producer::sendKiteRequest()
{
  if(m_options.sendInterest) {
    Interest interest(m_options.prefixPairs[0].producerSuffix);
    interest.setCanBePrefix(false);
    interest.setMustBeFresh(false);
    Name fh;
    fh.append(ndn::kite::KITE_KEYWORD);
    fh.append(m_options.prefixPairs[0].rvPrefix); 
    interest.setForwardingHint({fh});
    interest.setHopLimit(100);
    m_pendingInterest = m_face.expressInterest(interest,
                                              [this, &interest] (auto&&, const auto& data) { this->onData(interest, data); },
                                              [this] (auto&&, const auto& nack) { this->onNack(nack); },
                                              [this, &interest] (auto&&) { this->onTimeout(interest); });
  }
  Name producerPrefix(m_options.prefixPairs[0].rvPrefix);
  Request req;
  req.setRvPrefix(m_options.prefixPairs[0].rvPrefix);
  req.setProducerSuffix(m_options.prefixPairs[0].producerSuffix);
  req.setExpiration(m_options.lifetime);
  ndn::security::InterestSigner signer(m_keyChain);
  Interest interest = req.makeInterest(signer, ndn::security::signingByIdentity(Name(m_options.prefixPairs[0].producerSuffix)));

  NDN_LOG_DEBUG("Sending Request: " << interest.getName());

  m_face.expressInterest(interest,
                         bind(&Producer::onData, this, _1, _2),
                         bind(&Producer::onNack, this, _2),
                         bind(&Producer::onTimeout, this, _1));

  // m_nextUpdateEvent = m_scheduler.schedule(m_options.interval, [this] { sendKiteRequest(); });
}

void
Producer::onNack(const lp::Nack& nack)
{
  NDN_LOG_DEBUG("Received NACK: " << nack.getReason());
}

void
Producer::onTimeout(const Interest& interest)
{
  NDN_LOG_DEBUG("Request timed out: " << interest.getName());
}

} // namespace server
} // namespace ping
} // namespace ndn
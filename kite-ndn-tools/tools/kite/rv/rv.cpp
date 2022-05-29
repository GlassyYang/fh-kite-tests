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

#include "rv.hpp"

#include <ndn-cxx/kite/ack.hpp>
#include <ndn-cxx/kite/request.hpp>
#include <ndn-cxx/security/signing-helpers.hpp>
#include <ndn-cxx/util/logger.hpp>

namespace ndn {
namespace kite {
namespace rv {

NDN_LOG_INIT(kite.rv);

const char* CONF_PATH = "/usr/local/etc/ndn/rv.conf";

Rv::Rv(Face& face, KeyChain& keyChain, const Options& options)
  : m_options(options)
  , m_face(face)
  , m_keyChain(keyChain)
  , m_validator(face)
{
  m_validator.load(CONF_PATH);
}

void
Rv::start()
{
  m_registeredPrefix = m_face.setInterestFilter(
                       Name(m_options.prefixes[0]),
                       bind(&Rv::onInterest, this, _2),
                       [] (const auto&, const auto& reason) {
                         NDN_THROW(std::runtime_error("Failed to register prefix: " + reason));
                       });
  if(m_options.mpListen) {
  m_mpPrefix = m_face.setInterestFilter(
    Name(m_options.mpName),
    bind(&Rv::onMpInterest, this, _2));
  }
  if(m_options.sendInterest) {
    this->sendInterest();
  }
}

void
Rv::stop()
{
  m_registeredPrefix.unregister(); 
}


void
Rv::onInterest(const Interest& interest)
{
  afterReceive(interest.getName());

  m_validator.validate(interest,
                       std::bind(&Rv::onSuccess, this, _1),
                       std::bind(&Rv::onFailure, this, _1, _2));
}

void
Rv::onMpInterest(const Interest& interest)
{
  NDN_LOG_DEBUG("receive mp interest" << interest); 
  if (m_options.sendNack)
  {
    lp::Nack nack(interest);
    nack.setReason(lp::NackReason::NO_ROUTE);
    m_face.put(nack);
  }
  if (m_options.sendData) {
    Name producerPrefix(m_options.mpName);
    auto data = make_shared<Data>(interest.getName());
    std::string hintMessage = "this data is from rv. interest: ";
    hintMessage += interest.getName().toUri();
    shared_ptr<Buffer> buf = make_shared<Buffer>();
    for(char& chr : hintMessage) {
      buf->emplace_back(chr);
    }
    Block block(tlv::Content, buf);
    block.push_back(interest.wireEncode());
    data->setContent(block);
    m_keyChain.sign(*data, ndn::security::signingByIdentity(producerPrefix));
    m_face.put(*data);
  }
}



void
Rv::onSuccess(const Interest& interest)
{
  NDN_LOG_DEBUG("Verification success for: " << interest.getName());

  Request req;
  req.decode(interest);
  Ack ack;
  PrefixAnnouncement pa;
  pa.setAnnouncedName(req.getProducerPrefix());
  if (req.getExpiration()) {
    NDN_LOG_DEBUG("Has expiration: " << std::to_string(req.getExpiration()->count()) << " ms");
    pa.setExpiration(*req.getExpiration());
  }
  else {
    NDN_LOG_DEBUG("Expiration not set, setting to 1000 ms...");
    pa.setExpiration(1000_ms);
  }

  ack.setPrefixAnnouncement(pa);

  m_face.put(ack.makeData(interest, m_keyChain, ndn::security::signingByIdentity(m_options.prefixes[0])));
}

void
Rv::onFailure(const Interest& interest, const ValidationError& error)
{
  NDN_LOG_DEBUG("Verification failure for: " << interest.getName() << "\nError: " << error);
}

void
Rv::sendInterest()
{
  sleep(5);
  Interest interest(m_options.mpName + "");
  interest.setCanBePrefix(false);
  interest.setMustBeFresh(false);
  Delegation dl;
  dl.name = Name(m_options.prefixes[0]); 
  dl.preference = tlv::ContentType_KiteAck;
  DelegationList dll;
  dll.insert(dl);
  interest.setForwardingHint(dll);
  interest.setHopLimit(100);
  m_pendingInterest = m_face.expressInterest(interest,
                                             [this] (auto&&, const auto& data) { this->onData(data); },
                                             [this] (auto&&, const auto& nack) { this->onNack(nack); },
                                             [this] (auto&&) { this->onTimeout(); });
}
void
Rv::onData(const Data& data)
{
  std::cerr << "DATA: " << data.getName() << std::endl;
  const Block& block = data.getContent();
  std::cout.write(reinterpret_cast<const char*>(block.value()), block.value_size());
}

void
Rv::onNack(const lp::Nack& nack)
{
    lp::NackHeader header = nack.getHeader();
    std::cerr << "NACK: " << header.getReason() << std::endl;
}

void
Rv::onTimeout()
{
  std::cerr << "TIMEOUT" << std::endl;
}
} // namespace server
} // namespace ping
} // namespace ndn

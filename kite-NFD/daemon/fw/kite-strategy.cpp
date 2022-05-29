/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2020,  Regents of the University of California,
 *                           Arizona Board of Regents,
 *                           Colorado State University,
 *                           University Pierre & Marie Curie, Sorbonne University,
 *                           Washington University in St. Louis,
 *                           Beijing Institute of Technology,
 *                           The University of Memphis.
 *                           Harbin Institute of Technology
 *
 * This file is part of NFD (Named Data Networking Forwarding Daemon).
 * See AUTHORS.md for complete list of NFD authors and contributors.
 *
 * NFD is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * NFD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * NFD, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 * 
 * @author Yang Zhang <glassyyang@outlook.com>
 */

#include "kite-strategy.hpp"
#include "algorithm.hpp"
#include "common/global.hpp"
#include "common/logger.hpp"
#include "rib/service.hpp"
#include <iostream>

namespace nfd {
namespace fw{

NFD_LOG_INIT(KiteStrategy);
NFD_REGISTER_STRATEGY(KiteStrategy);


KiteStrategy::KiteStrategy(Forwarder& forwarder, const Name& name)
  : Strategy(forwarder), ProcessNackTraits(this)
{
  ParsedInstanceName parsed = parseInstanceName(name);
  if (!parsed.parameters.empty()) {
    NDN_THROW(std::invalid_argument("KiteStrategy does not accept parameters"));
  }
  if (parsed.version && *parsed.version != getStrategyName()[-1].toVersion()) {
    NDN_THROW(std::invalid_argument(
      "KiteStrategy does not support version " + to_string(*parsed.version)));
  }
  this->setInstanceName(makeInstanceName(name, getStrategyName()));
}

const Name& KiteStrategy::getStrategyName() {
  static Name strategyName("/localhost/nfd/strategy/kite/%FD%01");
  return strategyName;
}
  
void
KiteStrategy::afterReceiveInterest(const FaceEndpoint& ingress, const Interest& interest,
                      const shared_ptr<pit::Entry>& pitEntry) 
{
  const DelegationList& delegationList = interest.getForwardingHint();
  const Face& inFace = ingress.face;
  bool foundNextHops = false;
  if (!delegationList.empty())
  {
    auto kiteHint = delegationList.end();
    for (auto i = delegationList.begin(); i != delegationList.end(); i++)
    {
      NDN_LOG_DEBUG("forwarding hint preference: " << i->preference);
      if (i->preference == tlv::ContentType_KiteAck) {
          kiteHint = i;
          break;
      }
    }

    if(kiteHint != delegationList.end()) {
      NDN_LOG_DEBUG("kite hint found."); 
      // find fib entry by PIT entry
      const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
      const fib::NextHopList& nextHops = fibEntry.getNextHops();
      const auto& mpName = fibEntry.getPrefix();
      
      for(auto& nextHop : nextHops) {
        if(!isNextHopEligible(inFace, interest, nextHop, pitEntry)) {
          continue;
        }
        Face& outFace = nextHop.getFace();
        if(!foundNextHops) {
          foundNextHops = true;
          auto inRecordInfo = pitEntry->getInRecord(inFace)->insertStrategyInfo<KiteInterestStatus>().first;
          inRecordInfo->retrasmissionStage = InterestRetrasmissionStage::STRAIGHT_FORWARD;
          inRecordInfo->rvName = kiteHint->name;
        }
        NFD_LOG_DEBUG("send Interest=" << interest << " from=" << inFace.getId() <<
                  " to=" << outFace.getId());
        auto outRecord = this->sendInterest(pitEntry, outFace, interest);
        auto interestStatus = outRecord->insertStrategyInfo<KiteInterestStatus>().first;
        interestStatus->retrasmissionStage = InterestRetrasmissionStage::STRAIGHT_FORWARD;
        interestStatus->mpName = mpName;
      }
      // If cannot found nexthop to transmit, use rv forwarding hint transmiting interest.
      if(!foundNextHops) {
        // if cannot find fib entry, using rv name to find again.
        const Name& rvName = kiteHint->name;
        NFD_LOG_DEBUG("lookup nexthops by rv name: " << rvName.toUri());
        const fib::Entry& rvEntry = this->lookupFib(rvName);
        for(auto& nextHop : rvEntry.getNextHops()) {
          if(!isNextHopEligible(inFace, interest, nextHop, pitEntry)) {
            continue;
          }
          if(!foundNextHops) {
            foundNextHops = true;
          }
          Face& outFace = nextHop.getFace();
          NFD_LOG_DEBUG("send Interest=" << interest << " from=" << inFace.getId() <<
                    " to=" << outFace.getId());
          this->sendInterest(pitEntry, outFace, interest);
        }
        if (!foundNextHops) {
          // reject the interest
          NFD_LOG_DEBUG("NACK kite Interest=" << interest << " from=" << ingress << " noNextHop");
          lp::NackHeader nackHeader;
          nackHeader.setReason(lp::NackReason::NO_ROUTE);
          this->sendNack(pitEntry, ingress.face, nackHeader);
          this->rejectPendingInterest(pitEntry);
        }
      } else {
        const auto& entry = this->getMeasurements().get(mpName);
        auto pki = entry->insertStrategyInfo<KiteMobileProducerInfo>().first;
        pki->pitEntrys.insert(make_pair(interest.getName(), pitEntry));
      }
      return;
    }
  }

  const fib::Entry& fibEntry = this->lookupFib(*pitEntry);
  const fib::NextHopList& nextHops = fibEntry.getNextHops();
  if (nextHops.empty()) {
    NFD_LOG_DEBUG("NACK normal Interest=" << interest << " from=" << ingress << " noNextHop");
    lp::NackHeader nackHeader;
    nackHeader.setReason(lp::NackReason::NO_ROUTE);
    this->sendNack(pitEntry, ingress.face, nackHeader);
    this->rejectPendingInterest(pitEntry);
  }

  for (const auto& nexthop : nextHops) {
    if (!isNextHopEligible(inFace, interest, nexthop, pitEntry)) {
      continue;
    }
    if (!foundNextHops)
    {
      foundNextHops = true;
    } 
    Face& outFace = nexthop.getFace();
    NFD_LOG_DEBUG("send Interest=" << interest << " from=" << inFace.getId() <<
                  " to=" << outFace.getId());
    this->sendInterest(pitEntry, outFace, interest);
  }
  if(!foundNextHops) {
    // reject the interest
    NFD_LOG_DEBUG("NACK Interest=" << interest << " from=" << ingress << " noNextHop");
    lp::NackHeader nackHeader;
    nackHeader.setReason(lp::NackReason::NO_ROUTE);
    this->sendNack(pitEntry, ingress.face, nackHeader);
    this->rejectPendingInterest(pitEntry);
  }
}

void
KiteStrategy::afterReceiveNack(const FaceEndpoint& ingress, const lp::Nack& nack,
                  const shared_ptr<pit::Entry>& pitEntry) 
{
  auto outRecord = pitEntry->getOutRecord(ingress.face);
  if (outRecord != pitEntry->out_end())
  {
    auto interestStage = outRecord->getStrategyInfo<KiteInterestStatus>();
    if (interestStage != nullptr)
    {
      if (interestStage->retrasmissionStage == InterestRetrasmissionStage::STRAIGHT_FORWARD) {
       const Name& name = interestStage->mpName;
       NFD_LOG_DEBUG("NACK received for KITE mp interest. remove route" << interestStage->mpName);
       this->removePrefix(name, ingress.face.getId());
      }
    }
  }
  this->processNack(ingress.face, nack, pitEntry);
}

void
KiteStrategy::beforeSatisfyInterest(const shared_ptr<pit::Entry>& pitEntry,
                  const FaceEndpoint& ingress, const Data& data)
{
  if (data.getContentType() == tlv::ContentType_KiteAck){  
    ndn::kite::Ack ack(data);
    this->registPrefix(pitEntry, ack);
  } else {
    auto outRecord = pitEntry->getOutRecord(ingress.face);
    if (outRecord != pitEntry->out_end()) {
      eraseMeasurement(*outRecord, pitEntry->getName());
    }
  }
}

void
KiteStrategy::registPrefix(const shared_ptr<pit::Entry>& pitEntry, const ndn::kite::Ack& ack) {
  NFD_LOG_DEBUG("ACK received for KITE trace interest. regist route" << ack.getPrefixAnnouncement()->getAnnouncedName().toUri());
  for (const pit::InRecord& inRecord : pitEntry->getInRecords()) {
    if (inRecord.getFace().getScope() != ndn::nfd::FACE_SCOPE_LOCAL && inRecord.getExpiry() > time::steady_clock::now()) {
      auto& pa = *ack.getPrefixAnnouncement();
      runOnRibIoService([pitEntryWeak = weak_ptr<pit::Entry>{pitEntry}, inFaceId = inRecord.getFace().getId(), pa] {
        rib::Service::get().getRibManager().slAnnounce(pa, inFaceId, time::milliseconds(5_min),
          [&] (RibManager::SlAnnounceResult res) {
            NFD_LOG_DEBUG("Add kite-type route via PrefixAnnouncement with result=" << res);
          });
      });
      // retransmit pending interest to new mp
      const Name & mpName = pa.getAnnouncedName();
      auto entry = this->getMeasurements().get(mpName);
      if(entry == nullptr) {
        return;
      }
      this->getMeasurements().extendLifetime(*entry, time::duration_cast<time::nanoseconds>(pa.getExpiration()));
      auto mpInfo = entry->getStrategyInfo<KiteMobileProducerInfo>();
      auto& mpFace = inRecord.getFace();
      if(mpInfo != nullptr && mpInfo->faceIds.find(mpFace.getId()) == mpInfo->faceIds.end()) {
        mpInfo->faceIds.insert(mpFace.getId());
        for(auto& item : mpInfo->pitEntrys) {
          shared_ptr<pit::Entry> pitEntry = item.second.lock();
          if (pitEntry)
          {
            this->sendInterest(pitEntry, mpFace, pitEntry->getInterest());
          }
        }
      }
    }
  }
}

void
KiteStrategy::removePrefix(const Name& name, FaceId inFaceId)
{
  runOnRibIoService([name, inFaceId] {
    rib::Service::get().getRibManager().slRenew(name, inFaceId, 0_ms,
      [name] (RibManager::SlAnnounceResult res) {
        NFD_LOG_DEBUG("Delete route " << name.toUri() << " with result=" << res);
      });
  });
  auto entry = this->getMeasurements().get(name);
  auto kiteMpInfo = entry->getStrategyInfo<KiteMobileProducerInfo>();
  if (kiteMpInfo != nullptr)
  {
    kiteMpInfo->faceIds.erase(inFaceId);
  }
}

void
KiteStrategy::sendNackForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry, Face& outFace,
                              const lp::NackHeader& header)
{
  // inFace and outFace are the same under this situiation
  auto inRecord = pitEntry->getInRecord(outFace);
  if(!dealNack(*inRecord, pitEntry)) {
    NFD_LOG_DEBUG("cannot find eligible rv face to retransmit " << pitEntry->getInterest() << " send nack");
    this->sendNack(pitEntry, inRecord->getFace(), header);
    auto outRecord = pitEntry->getOutRecord(outFace);
    if(outRecord != pitEntry->out_end()) {
    eraseMeasurement(*outRecord, pitEntry->getName());
    }
  }
}

void
KiteStrategy::sendNacksForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry,
                              const lp::NackHeader& header)
{
  auto& inRecords = pitEntry->getInRecords();
  const auto& inRecord = std::find_if(inRecords.begin(), inRecords.end(), [&](const pit::InRecord& inRecord) {
    return dealNack(inRecord, pitEntry);
  });
  if(inRecord == inRecords.end()) {
    NFD_LOG_DEBUG("cannot find eligible rv face to retransmit " << pitEntry->getInterest() << " send nack");
    this->sendNacks(pitEntry, header);
    auto outRecord = pitEntry->out_begin();
    if (outRecord != pitEntry->out_end()) {
      eraseMeasurement(*outRecord, pitEntry->getName());
    }
  }
}

void 
KiteStrategy::eraseMeasurement(const pit::OutRecord& inRecord, const Name& interestName)
{
  auto interestStage = inRecord.getStrategyInfo<KiteInterestStatus>();
  if(interestStage != nullptr) {
    const Name& mpName = interestStage->mpName;
    auto entry = this->getMeasurements().get(mpName);
    auto pki = entry->getStrategyInfo<KiteMobileProducerInfo>();
    if(pki != nullptr) {
      pki->pitEntrys.erase(interestName);
    }
  }
}

bool
KiteStrategy::dealNack(const pit::InRecord& inRecord, const shared_ptr<pit::Entry>& pitEntry)
{ 
  auto& inRecords = pitEntry->getInRecords();
  auto& interest = pitEntry->getInterest();
  auto interestStatus = inRecord.getStrategyInfo<KiteInterestStatus>();
  if(interestStatus != nullptr) {
    if (interestStatus->retrasmissionStage == InterestRetrasmissionStage::STRAIGHT_FORWARD) {
      interestStatus->retrasmissionStage = InterestRetrasmissionStage::RV;
      Name& name = interestStatus->rvName;
      // retransmission
      auto& fib = this->lookupFib(name);
      auto nextHops = fib.getNextHops();
      bool foundNextHops = false;
      auto& outRecords = pitEntry->getOutRecords();
      for(auto& nextHop : nextHops) {
        auto& outFace = nextHop.getFace();
        auto foundIn = std::find_if(inRecords.begin(), inRecords.end(), [&](const pit::InRecord& inRecord){
            auto& inFace = inRecord.getFace();
            return inFace.getId() == outFace.getId();
        });
        if(foundIn != inRecords.end()) {
          continue;
        }
        auto foundOut = std::find_if(outRecords.begin(), outRecords.end(), [&](const pit::OutRecord& outRecord){
          auto& lastOutFace = outRecord.getFace();
          return lastOutFace.getId() == outFace.getId();
        });
        if(foundOut != outRecords.end()) {
          continue;
        }
        if(!foundNextHops) {
          foundNextHops = true;
        }
        NFD_LOG_DEBUG("send Interest=" << interest << " from=" << inRecord.getFace().getId() <<
                    " to=" << nextHop.getFace().getId());
        this->sendInterest(pitEntry, nextHop.getFace(), interest);
      }
      return foundNextHops;
    }
  }
  return false;
}
}
}
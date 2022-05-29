/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014-2022,  Regents of the University of California,
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

#ifndef NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP
#define NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP

#include "fw/strategy.hpp"
#include "process-nack-traits.hpp"

#include <ndn-cxx/lp/prefix-announcement-header.hpp>
#include <ndn-cxx/kite/ack.hpp>
namespace nfd
{
namespace fw
{
class KiteStrategy : public Strategy,
                      public ProcessNackTraits<KiteStrategy>
{
public:
  explicit KiteStrategy(Forwarder &forwarder, const Name &name = getStrategyName());

  static const Name &
  getStrategyName();
  enum class InterestRetrasmissionStage {

    /** \brief Interest is forwarded to producer
     */
    STRAIGHT_FORWARD,
    /** \brief Interest is forwarded to RV
     */
    RV,
  };

  class KiteInterestStatus : public StrategyInfo
  {

  public:
    
    static constexpr int
    getTypeId()
    {
      return 1132;
    }

  public:
    InterestRetrasmissionStage retrasmissionStage;
    Name rvName;
    Name mpName;
  };

  class KiteMobileProducerInfo: public StrategyInfo
  {
  public:
    
    static constexpr int
    getTypeId()
    {
      return 1134;
    }

  public:
    Name mpName;
    std::unordered_map<Name, weak_ptr<pit::Entry>> pitEntrys;
    std::unordered_set<face::FaceId> faceIds;
  };

  public: // triggers
            void
            afterReceiveInterest(const FaceEndpoint &ingress, const Interest &interest,
                                const shared_ptr<pit::Entry> &pitEntry) override;

  void
  afterReceiveNack(const FaceEndpoint &ingress, const lp::Nack &nack,
                    const shared_ptr<pit::Entry> &pitEntry) override;

  void
  beforeSatisfyInterest(const shared_ptr<pit::Entry> &pitEntry,
                        const FaceEndpoint &ingress, const Data &data) override;

private:
  void
  registPrefix(const shared_ptr<pit::Entry> &pitEntry, const ndn::kite::Ack &ack);
  friend ProcessNackTraits<KiteStrategy>;

  void
  removePrefix(const Name& name, FaceId inFaceId);
  void
  sendNackForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry, Face& outFace,
                               const lp::NackHeader& header) override;

  void
  sendNacksForProcessNackTraits(const shared_ptr<pit::Entry>& pitEntry,
                                const lp::NackHeader& header) override;
  
  bool
  dealNack(const pit::InRecord& inRecord, const shared_ptr<pit::Entry>& pitEntry);

  inline void 
  eraseMeasurement(const pit::OutRecord& outRecord, const Name& interestName);
};
}
}

#endif // NFD_DAEMON_FW_SELF_LEARNING_STRATEGY_HPP

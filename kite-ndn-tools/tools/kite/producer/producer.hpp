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

#ifndef NDN_TOOLS_KITE_PRODUCER_HPP
#define NDN_TOOLS_KITE_PRODUCER_HPP

#include "core/common.hpp"

namespace ndn
{
  namespace kite
  {
    namespace producer
    {

      /**
       * @brief RV prefix, producer suffix pair
       */
      struct PrefixPair
      {
        Name rvPrefix;       //!< RV prefix
        Name producerSuffix; //!< producer suffix
      };

      /**
       * @brief Options for Producer
       */
      struct Options
      {
        std::vector<PrefixPair> prefixPairs; //!< mobile producer prefixes
        time::milliseconds interval;         //!< update interval
        time::milliseconds lifetime;         //!< Request lifetime
        int action;                          // action. 0 means send data, 1 means send ack, 2 means do nothing
        bool sendInterest;                   // send interest. before mp send trace interest, send a comsuner interest.
                                             // after regist cusseed, mp will receive thr interest.
      };

      /**
       * @brief KITE mobile producer
       */
      class Producer : noncopyable
      {
      public:
        Producer(Face &face, KeyChain &keyChain, const Options &options);

        /**
         * @brief Signals when Interest received
         *
         * @param name incoming interest name
         */
        signal::Signal<Producer, Name> afterReceive;

        /**
         * @brief Sets the Interest filter
         *
         * @note This method is non-blocking and caller need to call face.processEvents()
         */
        void
        start();

        /**
         * @brief Unregister set interest filter
         */
        void
        stop();

        /**
         * @brief Send a new KITE request
         */
        void
        sendKiteRequest();

      private:
        /**
         * @brief Called when Interest received
         *
         * @param interest incoming Interest
         */
        void
        onInterest(const InterestFilter &filter, const Interest &interest);

        /**
         * @brief Called when Data received
         *
         * @param data incoming data
         */
        void
        onData(const Interest &interest, const Data &data);

        /**
         * @brief Process a potential KITE acknowledgment
         *
         * @param interest a potential encoded KITE acknowledgment
         */
        // void
        // processKiteAck(const Data& data);

        /**
         * @brief Called when a Nack is received in response to a ping
         *
         * @param interest NDN interest
         * @param nack returned nack
         * @param seq ping sequence number
         * @param sendTime time ping sent
         */
        void
        onNack(const lp::Nack &nack);

        /**
         * @brief Called when ping timed out
         *
         * @param interest NDN interest
         * @param seq ping sequence number
         */
        void
        onTimeout(const Interest &interest);

      private:
        const Options &m_options;
        Face &m_face;
        KeyChain &m_keyChain;
        RegisteredPrefixHandle m_registeredPrefix;
        Scheduler m_scheduler;
        scheduler::ScopedEventId m_nextUpdateEvent;
        PendingInterestHandle m_pendingInterest;
      };

    } // namespace producer
  }   // namespace kite
} // namespace ndn

#endif // NDN_TOOLS_KITE_PRODUCER_HPP

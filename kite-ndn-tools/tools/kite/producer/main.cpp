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

#include "core/common.hpp"
#include "core/version.hpp"

#include "producer.hpp"

namespace ndn {
namespace kite {
namespace producer {

class Runner : noncopyable
{
public:
  explicit
  Runner(const Options& options)
    : m_producer(m_face, m_keyChain, options)
    , m_signalSet(m_face.getIoService(), SIGINT, SIGQUIT)
  {
    m_signalSet.async_wait(bind(&Runner::afterSignal, this, _1, _2));
  }

  int
  run()
  {
    try {
      m_producer.start();
      m_face.processEvents();
    }
    catch (const std::exception& e) {
      std::cout << e.what() << std::endl;
      return 1;
    }

    return 0;
  }

private:
  void
  cancel()
  {
    m_producer.stop();
  }

  void
  afterSignal(const boost::system::error_code& errorCode, int sigNum)
  {
    if (errorCode == boost::asio::error::operation_aborted) {
      return;
    }

    switch (sigNum) {
      case SIGINT:
        m_producer.sendKiteRequest();
        break;
      case SIGQUIT:
        cancel();
        exit(0);
        break;
      default:
        break;
    }
    m_signalSet.async_wait(bind(&Runner::afterSignal, this, _1, _2));
  }

private:
  Face m_face;
  KeyChain m_keyChain;
  Producer m_producer;

  boost::asio::signal_set m_signalSet;
};

static time::milliseconds
getDefaultInterval()
{
  return time::milliseconds(1000);
}

static time::milliseconds
getDefaultLifetime()
{
  return time::milliseconds(1000);
}

static void
usage(const boost::program_options::options_description& options)
{
  std::cout << "Usage: kiteproducer [options]\n"
            << options;
  exit(2);
}

static int
main(int argc, char* argv[])
{
  Options options;
  options.interval = time::milliseconds(getDefaultInterval());
  options.lifetime = time::milliseconds(getDefaultLifetime());
  options.action = 0;
  std::string identifier;

  namespace po = boost::program_options;

  po::options_description visibleOptDesc("Allowed options");
  visibleOptDesc.add_options()
    ("help,h", "print this message and exit")
    ("version,V", "display version and exit")
    ("rv-prefix,r", po::value<std::string>(), "RV prefix")
    ("producer-suffix,p", po::value<std::string>(), "producer suffix")
    ("interval,i", po::value<int>(),
                   ("set interval in milliseconds (default " +
                   std::to_string(getDefaultInterval().count()) + " ms").c_str())
    ("lifetime,l", po::value<int>(),
                   ("set lifetime in milliseconds (default " +
                   std::to_string(getDefaultLifetime().count()) + " ms").c_str())
    ("action,a", po::value<int>(), "set producer action. 0 means send data, 1 means send nack, 2 means send nothing. default 0")
    ("send-interest,s", po::bool_switch(&options.sendInterest), "send interest. before mp send trace interest, send a comsuner interest. after regist succeed, mp will receive thr interest.");
  ;

  po::options_description optDesc("Allowed options");
  optDesc.add(visibleOptDesc);

  try {
    po::variables_map optVm;
    po::store(po::command_line_parser(argc, argv).options(optDesc).run(), optVm);
    po::notify(optVm);

    PrefixPair prefixPair;

    if (optVm.count("help") > 0) {
      usage(visibleOptDesc);
    }

    if (optVm.count("version") > 0) {
      std::cout << "KITE producer " << tools::VERSION << std::endl;
      exit(0);
    }

    if (optVm.count("rv-prefix") > 0) {
      prefixPair.rvPrefix = Name(optVm["rv-prefix"].as<std::string>());
    }
    else {
      std::cerr << "ERROR: No RV prefix specified" << std::endl;
      usage(visibleOptDesc);
    }

    if (optVm.count("producer-suffix") > 0) {
      prefixPair.producerSuffix = Name(optVm["producer-suffix"].as<std::string>());
    }
    else {
      std::cerr << "ERROR: No producer suffix specified" << std::endl;
      usage(visibleOptDesc);
    }

    options.prefixPairs.push_back(prefixPair);

    if (optVm.count("interval") > 0) {
      options.interval = time::milliseconds(optVm["interval"].as<int>());
    }

    if (optVm.count("lifetime") > 0) {
      options.lifetime = time::milliseconds(optVm["lifetime"].as<int>());
    }
    if(optVm.count("action") > 0) {
      options.action = optVm["action"].as<int>();
      if(options.action < 0 || options. action > 2) {
        std::cerr << "wrong action " << options.action << ". should be 0(do nothing), 1(send data) or 2(send nack)" << std::endl;
      }
    }
  }
  catch (const po::error& e) {
    std::cerr << "ERROR: " << e.what() << std::endl;
    usage(visibleOptDesc);
  }

  return Runner(options).run();
}

} // namespace producer
} // namespace kite
} // namespace ndn

int
main(int argc, char* argv[])
{
  return ndn::kite::producer::main(argc, argv);
}

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
#include "ndn-cxx/security/interest-signer.hpp"
#include "ndn-cxx/util/dummy-client-face.hpp"

#include "tests/boost-test.hpp"
#include "tests/unit/io-key-chain-fixture.hpp"

namespace ndn {
namespace kite {
namespace tests {

// using ::ndn::tests::makeInterest;

class RequestFixture : public ndn::tests::IoKeyChainFixture
{
public:
  RequestFixture()
    : rvPrefix("/rv")
    , producerSuffix("/alice")
    , signer(m_keyChain)
  {
  }

protected:
  const Name rvPrefix;
  const Name producerSuffix;
  security::InterestSigner signer;
};

BOOST_FIXTURE_TEST_SUITE(TestRequest, RequestFixture)

static Name
makePartialKiteName()
{
  return Name(
    "0711 rv-prefix=/rv 08027276"
    "     keyword 20044B495445"
    "     suffix=/alice 0805616C696365"_block);
}

static Interest
makeRequestInterest()
{
  const uint8_t WIRE[] = {
    0x05, 0x78, // Interest
          0x07, 0x33, // Name
                0x08, 0x02, // RV prefix: "rv"
                      0x72, 0x76,
                0x20, 0x04, // Keyword: "32=KITE"
                      0x4b, 0x49, 0x54, 0x45,
                0x08, 0x05, // Producer suffix: "alice"
                      0x61, 0x6c, 0x69, 0x63, 0x65,
                0x02, 0x20, // ParametersSha256DigestComponent
                      0x6e, 0x90, 0x00, 0xa3, 0xd2, 0x2e, 0x44, 0x8d,
                      0x1d, 0x3c, 0x35, 0xc0, 0x52, 0xef, 0xc0, 0x26,
                      0x48, 0xfd, 0x42, 0xd0, 0x14, 0x52, 0xc8, 0x14,
                      0x2f, 0xcc, 0xc8, 0xba, 0x40, 0xf8, 0x42, 0x8c,
          0x0a, 0x04, // Nonce
                0x4c, 0x1e, 0xcb, 0x4a,
          0x24, 0x00, // ApplicationParameters: empty
          0x2c, 0x17, // InterestSignatureInfo
                0x1b, 0x01, // SignatureType
                      0x00,
                0x26, 0x08, // SignatureNonce
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                0x28, 0x08, // SignatureTime
                      0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
          0x2e, 0x20, // InterestSignatureValue
                0x12, 0x47, 0x1a, 0xe0, 0xf8, 0x72, 0x3a, 0xc1,
                0x15, 0x6c, 0x37, 0x0a, 0x38, 0x71, 0x1e, 0xbe,
                0xbf, 0x28, 0x17, 0xde, 0x9b, 0x2d, 0xd9, 0x4e,
                0x9b, 0x7e, 0x62, 0xf1, 0x17, 0xb8, 0x76, 0xc1,
  };

  return Interest(Block{WIRE});
}

shared_ptr<Interest>
makeInterest(const Name& name, bool canBePrefix = false,
             optional<time::milliseconds> lifetime = nullopt,
             optional<Interest::Nonce> nonce = nullopt)
{
  auto interest = std::make_shared<Interest>(name);
  interest->setCanBePrefix(canBePrefix);
  if (lifetime) {
    interest->setInterestLifetime(*lifetime);
  }
  interest->setNonce(nonce);
  return interest;
}

BOOST_AUTO_TEST_CASE(DecodeGood)
{
  Request req;
  req.decode(makeRequestInterest());
  BOOST_CHECK_EQUAL(req.getRvPrefix(), rvPrefix);
  BOOST_CHECK_EQUAL(req.getProducerSuffix(), producerSuffix);
}

BOOST_AUTO_TEST_CASE(DecodeBad)
{
  Name testName;
  Request req;

  // name follows KITE request name specifications, but not signed
  testName = rvPrefix;
  testName.append(KITE_KEYWORD);
  testName.append(producerSuffix);
  Interest interest1 = *makeInterest(testName);
  BOOST_CHECK_THROW(req.decode(interest1), Request::Error);

  // signed, but no nonce and no timestamp
  security::SigningInfo si;
  si.setSignedInterestFormat(security::SignedInterestFormat::V03);
  security::InterestSigner signer(m_keyChain);
  Interest interest2 = *makeInterest(testName);
  signer.makeSignedInterest(interest2, si, security::InterestSigner::WantSeqNum);
  BOOST_CHECK_THROW(req.decode(interest2), Request::Error);

  // signed with nonce and timestamp, but no keyword
  Interest interest3 = *makeInterest(rvPrefix);
  signer.makeSignedInterest(interest3, si,
                            security::InterestSigner::WantNonce | security::InterestSigner::WantTime);
  BOOST_CHECK_THROW(req.decode(interest3), Request::Error);

  // signed with nonce and timestamp, but no RV prefix
  Interest interest4 = *makeInterest(Name().append(KITE_KEYWORD));
  signer.makeSignedInterest(interest4, si,
                            security::InterestSigner::WantNonce | security::InterestSigner::WantTime);
  BOOST_CHECK_THROW(req.decode(interest4), Request::Error);

  // signed with nonce and timestamp, but no producer suffix
  testName = rvPrefix;
  testName.append(KITE_KEYWORD);
  Interest interest5 = *makeInterest(testName);
  signer.makeSignedInterest(interest5, si,
                            security::InterestSigner::WantNonce | security::InterestSigner::WantTime);
  BOOST_CHECK_THROW(req.decode(interest5), Request::Error);
}

BOOST_AUTO_TEST_CASE(Encode)
{
  Request req;
  req.setRvPrefix(rvPrefix);
  req.setProducerSuffix(producerSuffix);
  security::SigningInfo si;
  si.setSignedInterestFormat(security::SignedInterestFormat::V03);
  Interest interest = req.makeInterest(signer, si);
  BOOST_CHECK_EQUAL(interest.getName().size(), 4);
  BOOST_CHECK_EQUAL(interest.getName().getPrefix(3), makePartialKiteName());
}

BOOST_AUTO_TEST_CASE(EncodeDecodeWithExpiration)
{
  Request req1;
  req1.setRvPrefix(rvPrefix);
  req1.setProducerSuffix(producerSuffix);
  time::milliseconds expiration = 1000_ms;
  req1.setExpiration(expiration);
  BOOST_CHECK_EQUAL(*req1.getExpiration(), expiration);
  security::SigningInfo si;
  si.setSignedInterestFormat(security::SignedInterestFormat::V03);
  Interest interest = req1.makeInterest(signer, si);

  Request req2;
  req2.decode(interest);

  BOOST_CHECK_EQUAL(req2.getRvPrefix(), rvPrefix);
  BOOST_CHECK_EQUAL(req2.getProducerSuffix(), producerSuffix);
  BOOST_CHECK_EQUAL(*req2.getExpiration(), expiration);
}

BOOST_AUTO_TEST_SUITE_END() // TestRequest

} // namespace tests
} // namespace kite
} // namespace ndn

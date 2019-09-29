/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <libp2p/protocol_muxer/multiselect/message_manager.hpp>

#include <string_view>
#include <vector>

#include <gtest/gtest.h>
#include <libp2p/common/types.hpp>
#include <libp2p/peer/peer_id.hpp>
#include <libp2p/protocol_muxer/multiselect/multiselect.hpp>
#include <testutil/outcome.hpp>

using libp2p::common::ByteArray;
using libp2p::multi::Multihash;
using libp2p::multi::UVarint;
using libp2p::peer::Protocol;
using libp2p::protocol_muxer::MessageManager;
using libp2p::protocol_muxer::Multiselect;

using MessageType = MessageManager::MultiselectMessage::MessageType;

class MessageManagerTest : public ::testing::Test {
  static constexpr std::string_view kMultiselectHeaderProtocol =
      "/multistream-select/1.0.0";

 public:
  const std::vector<Protocol> kDefaultProtocols{
      "/plaintext/1.0.0", "/ipfs-dht/0.2.3", "/http/w3id.org/http/1.1"};
  static constexpr uint64_t kProtocolsVarintsSize = 3;
  static constexpr uint64_t kProtocolsListBytesSize = 60;
  static constexpr uint64_t kProtocolsNumber = 3;

  const ByteArray kOpeningMsg = []() -> ByteArray {
    ByteArray buffer =
        UVarint{kMultiselectHeaderProtocol.size() + 1}.toVector();
    buffer.insert(buffer.end(), kMultiselectHeaderProtocol.begin(),
                  kMultiselectHeaderProtocol.end());
    buffer.push_back('\n');
    return buffer;
  }();

  const ByteArray kLsMsg = []() -> ByteArray {
    ByteArray buffer = UVarint{3}.toVector();
    std::string_view ls = "ls\n";
    buffer.insert(buffer.end(), ls.begin(), ls.end());
    return buffer;
  }();

  const ByteArray kNaMsg = []() -> ByteArray {
    ByteArray buffer = UVarint{3}.toVector();
    std::string_view na = "na\n";
    buffer.insert(buffer.end(), na.begin(), na.end());
    return buffer;
  }();

  const ByteArray kProtocolMsg = [this]() -> ByteArray {
    ByteArray buffer = UVarint{kDefaultProtocols[0].size() + 1}.toVector();
    buffer.insert(buffer.end(), kDefaultProtocols[0].begin(),
                  kDefaultProtocols[0].end());
    buffer.push_back('\n');
    return buffer;
  }();

  const ByteArray kProtocolsMsg = [this]() -> ByteArray {
    ByteArray buffer = UVarint{kProtocolsVarintsSize}.toVector();
    {
      auto &&tmp = UVarint{kProtocolsListBytesSize}.toVector();
      buffer.insert(buffer.end(), tmp.begin(), tmp.end());
    }
    {
      auto &&tmp = UVarint{kProtocolsNumber}.toVector();
      buffer.insert(buffer.end(), tmp.begin(), tmp.end());
    }
    buffer.push_back('\n');
    {
      auto &&tmp = UVarint{kDefaultProtocols[0].size() + 1}.toVector();
      buffer.insert(buffer.end(), tmp.begin(), tmp.end());
    }
    buffer.insert(buffer.end(), kDefaultProtocols[0].begin(),
                  kDefaultProtocols[0].end());
    buffer.push_back('\n');

    {
      auto &&tmp = UVarint{kDefaultProtocols[1].size() + 1}.toVector();
      buffer.insert(buffer.end(), tmp.begin(), tmp.end());
    }
    buffer.insert(buffer.end(), kDefaultProtocols[1].begin(),
                  kDefaultProtocols[1].end());
    buffer.push_back('\n');
    {
      auto &&tmp = UVarint{kDefaultProtocols[2].size() + 1}.toVector();
      buffer.insert(buffer.end(), tmp.begin(), tmp.end());
    }
    buffer.insert(buffer.end(), kDefaultProtocols[2].begin(),
                  kDefaultProtocols[2].end());
    buffer.push_back('\n');

    return buffer;
  }();
};

/**
 * @given message manager
 * @when getting an opening message from it
 * @then well-formed opening message is returned
 */
TEST_F(MessageManagerTest, ComposeOpeningMessage) {
  auto opening_msg = MessageManager::openingMsg();
  ASSERT_EQ(opening_msg, kOpeningMsg);
}

/**
 * @given message manager
 * @when getting an ls message from it
 * @then well-formed ls message is returned
 */
TEST_F(MessageManagerTest, ComposeLsMessage) {
  auto ls_msg = MessageManager::lsMsg();
  ASSERT_EQ(ls_msg, kLsMsg);
}

/**
 * @given message manager
 * @when getting an na message from it
 * @then well-formed na message is returned
 */
TEST_F(MessageManagerTest, ComposeNaMessage) {
  auto na_msg = MessageManager::naMsg();
  ASSERT_EQ(na_msg, kNaMsg);
}

/**
 * @given message manager @and protocol
 * @when getting a protocol message from it
 * @then well-formed protocol message is returned
 */
TEST_F(MessageManagerTest, ComposeProtocolMessage) {
  auto protocol_msg = MessageManager::protocolMsg(kDefaultProtocols[0]);
  ASSERT_EQ(protocol_msg, kProtocolMsg);
}

/**
 * @given message manager @and protocols
 * @when getting a protocols message from it
 * @then well-formed protocols message is returned
 */
TEST_F(MessageManagerTest, ComposeProtocolsMessage) {
  auto protocols_msg = MessageManager::protocolsMsg(kDefaultProtocols);
  ASSERT_EQ(protocols_msg, kProtocolsMsg);
}

/**
 * @given message manager @and ls msg
 * @when parsing it with a ParseConstMsg
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseConstLs) {
  std::string_view msg = "ls\n";
  ByteArray parsable_ls_msg(msg.begin(), msg.end());
  auto msg_opt = MessageManager::parseConstantMsg(parsable_ls_msg);
  ASSERT_TRUE(msg_opt);
  ASSERT_EQ(msg_opt.value().type, MessageType::LS);
}

/**
 * @given message manager @and na msg
 * @when parsing it with a ParseConstMsg
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseConstNa) {
  std::string_view msg = "na\n";
  ByteArray parsable_na_msg(msg.begin(), msg.end());
  auto msg_opt = MessageManager::parseConstantMsg(parsable_na_msg);
  ASSERT_TRUE(msg_opt);
  ASSERT_EQ(msg_opt.value().type, MessageType::NA);
}

/**
 * @given message manager @and protocol msg
 * @when parsing it with a ParseConstMsg
 * @then parse fails
 */
TEST_F(MessageManagerTest, ParseConstFail) {
  EXPECT_FALSE(MessageManager::parseConstantMsg(kProtocolMsg));
}

/**
 * @given message manager @and part of message with protocols header
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocolsHeader) {
  auto protocols_header = gsl::make_span(kProtocolsMsg);
  EXPECT_OUTCOME_TRUE(
      parsed_header,
      MessageManager::parseProtocolsHeader(protocols_header.subspan(1)))
  ASSERT_EQ(parsed_header.number_of_protocols, kProtocolsNumber);
  ASSERT_EQ(parsed_header.size_of_protocols, kProtocolsListBytesSize);
}

/**
 * @given message manager @and part of message with protocols
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocols) {
  auto protocols = gsl::make_span(kProtocolsMsg);
  EXPECT_OUTCOME_TRUE(
      parsed_protocols,
      MessageManager::parseProtocols(protocols.subspan(4), kProtocolsNumber))
  ASSERT_EQ(parsed_protocols.type, MessageType::PROTOCOLS);
  ASSERT_EQ(parsed_protocols.protocols, kDefaultProtocols);
}

/**
 * @given message manager @and protocol msg
 * @when parsing it as a protocols message
 * @then parse fails
 */
TEST_F(MessageManagerTest, ParseProtocolsFail) {
  EXPECT_FALSE(MessageManager::parseProtocols(kProtocolMsg, kProtocolsNumber));
}

/**
 * @given message manager @and protocol msg
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseProtocol) {
  auto protocol = gsl::make_span(kProtocolMsg);
  EXPECT_OUTCOME_TRUE(parsed_protocol,
                      MessageManager::parseProtocol(protocol.subspan(1)))
  ASSERT_EQ(parsed_protocol.type, MessageType::PROTOCOL);
  ASSERT_EQ(parsed_protocol.protocols[0], kDefaultProtocols[0]);
}

/**
 * @given message manager @and opening msg
 * @when parsing it
 * @then parse is successful
 */
TEST_F(MessageManagerTest, ParseOpening) {
  auto opening = gsl::make_span(kOpeningMsg);
  EXPECT_OUTCOME_TRUE(parsed_protocol,
                      MessageManager::parseProtocol(opening.subspan(1)))
  ASSERT_EQ(parsed_protocol.type, MessageType::OPENING);
}

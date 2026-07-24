#include "test_framework.h"

#include <network/interface/i_network_service.h>
#include <network/logic/network_protocol.h>
#include <array>

AXIS_TEST_CASE("NetworkConfig requires a secure provider by default")
{
    const NetworkConfig config;
    AXIS_CHECK(config.securityMode == NetworkSecurityMode::RequireSecure);
    AXIS_CHECK(!config.allowAnyInterfaceFallback);
}

AXIS_TEST_CASE("NetworkProtocol round trips a bounded user message")
{
    const std::array<uint8_t, 4> input = {1, 2, 3, 4};
    std::vector<uint8_t> encoded;
    AXIS_CHECK(NetworkProtocol::Encode(NetworkPacketKind::UserMessage, 7, input.data(), input.size(), 64, encoded));

    NetworkProtocolPacket decoded;
    AXIS_CHECK(NetworkProtocol::Decode(encoded.data(), encoded.size(), 64, decoded));
    AXIS_CHECK(decoded.kind == NetworkPacketKind::UserMessage);
    AXIS_CHECK(decoded.sequence == 7);
    AXIS_CHECK(decoded.payload == std::vector<uint8_t>(input.begin(), input.end()));
}

AXIS_TEST_CASE("NetworkProtocol rejects invalid headers and payload lengths")
{
    const std::array<uint8_t, 2> input = {9, 8};
    std::vector<uint8_t> encoded;
    AXIS_CHECK(NetworkProtocol::Encode(NetworkPacketKind::Replication, 3, input.data(), input.size(), 64, encoded));

    NetworkProtocolPacket decoded;
    encoded[0] ^= 0xFF;
    AXIS_CHECK(!NetworkProtocol::Decode(encoded.data(), encoded.size(), 64, decoded));
    encoded[0] ^= 0xFF;
    encoded.back() = 0;
    encoded.pop_back();
    AXIS_CHECK(!NetworkProtocol::Decode(encoded.data(), encoded.size(), 64, decoded));
}

AXIS_TEST_CASE("NetworkProtocol enforces configured payload limits and nonzero sequences")
{
    const std::array<uint8_t, 8> input = {};
    std::vector<uint8_t> encoded;
    AXIS_CHECK(!NetworkProtocol::Encode(NetworkPacketKind::UserMessage, 0, input.data(), input.size(), 64, encoded));
    AXIS_CHECK(!NetworkProtocol::Encode(NetworkPacketKind::UserMessage, 1, input.data(), input.size(), 4, encoded));
    AXIS_CHECK(NetworkProtocol::Encode(NetworkPacketKind::UserMessage, 1, input.data(), input.size(), 64, encoded));

    NetworkProtocolPacket decoded;
    AXIS_CHECK(!NetworkProtocol::Decode(encoded.data(), encoded.size(), 4, decoded));
}

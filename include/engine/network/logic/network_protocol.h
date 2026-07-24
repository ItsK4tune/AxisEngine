#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

enum class NetworkPacketKind : uint8_t
{
    UserMessage = 1,
    Replication = 2
};

struct NetworkProtocolPacket
{
    NetworkPacketKind kind = NetworkPacketKind::UserMessage;
    uint32_t sequence = 0;
    std::vector<uint8_t> payload;
};

class NetworkProtocol
{
public:
    static constexpr uint32_t Magic = 0x41584E50u;
    static constexpr uint16_t Version = 1;
    static constexpr size_t HeaderBytes = 16;

    static bool Encode(NetworkPacketKind kind, uint32_t sequence, const void* payload, size_t payloadSize,
                       size_t maxPayloadBytes, std::vector<uint8_t>& output);
    static bool Decode(const void* data, size_t size, size_t maxPayloadBytes, NetworkProtocolPacket& output);
};

#include <network/logic/network_protocol.h>

#include <cstring>
#include <limits>
#include <new>

namespace
{
void ProtocolWriteU16(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(value & 0xFF));
}

void ProtocolWriteU32(std::vector<uint8_t>& output, uint32_t value)
{
    output.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(value & 0xFF));
}

bool ProtocolReadU16(const uint8_t*& cursor, const uint8_t* end, uint16_t& value)
{
    if (static_cast<size_t>(end - cursor) < sizeof(uint16_t))
        return false;
    value = static_cast<uint16_t>((static_cast<uint16_t>(cursor[0]) << 8) | cursor[1]);
    cursor += sizeof(uint16_t);
    return true;
}

bool ProtocolReadU32(const uint8_t*& cursor, const uint8_t* end, uint32_t& value)
{
    if (static_cast<size_t>(end - cursor) < sizeof(uint32_t))
        return false;
    value = (static_cast<uint32_t>(cursor[0]) << 24) | (static_cast<uint32_t>(cursor[1]) << 16) |
            (static_cast<uint32_t>(cursor[2]) << 8) | static_cast<uint32_t>(cursor[3]);
    cursor += sizeof(uint32_t);
    return true;
}
}

bool NetworkProtocol::Encode(NetworkPacketKind kind, uint32_t sequence, const void* payload, size_t payloadSize,
                             size_t maxPayloadBytes, std::vector<uint8_t>& output)
{
    if ((!payload && payloadSize != 0) || sequence == 0 || payloadSize > maxPayloadBytes ||
        payloadSize > (std::numeric_limits<uint32_t>::max)())
        return false;
    if (kind != NetworkPacketKind::UserMessage && kind != NetworkPacketKind::Replication)
        return false;

    try
    {
        output.clear();
        output.reserve(HeaderBytes + payloadSize);
        ProtocolWriteU32(output, Magic);
        ProtocolWriteU16(output, Version);
        output.push_back(static_cast<uint8_t>(kind));
        output.push_back(0);
        ProtocolWriteU32(output, sequence);
        ProtocolWriteU32(output, static_cast<uint32_t>(payloadSize));
        if (payloadSize != 0)
        {
            const auto* bytes = static_cast<const uint8_t*>(payload);
            output.insert(output.end(), bytes, bytes + payloadSize);
        }
    }
    catch (const std::bad_alloc&)
    {
        output.clear();
        return false;
    }
    return true;
}

bool NetworkProtocol::Decode(const void* data, size_t size, size_t maxPayloadBytes,
                             NetworkProtocolPacket& output)
{
    if (!data || size < HeaderBytes)
        return false;
    const auto* cursor = static_cast<const uint8_t*>(data);
    const auto* end = cursor + size;
    uint32_t magic = 0;
    uint16_t version = 0;
    uint32_t sequence = 0;
    uint32_t payloadSize = 0;
    if (!ProtocolReadU32(cursor, end, magic) || !ProtocolReadU16(cursor, end, version) || cursor == end)
        return false;
    const auto kind = static_cast<NetworkPacketKind>(*cursor++);
    if (cursor == end || *cursor++ != 0 || !ProtocolReadU32(cursor, end, sequence) ||
        !ProtocolReadU32(cursor, end, payloadSize))
        return false;
    if (magic != Magic || version != Version || sequence == 0 || payloadSize > maxPayloadBytes ||
        payloadSize != static_cast<size_t>(end - cursor))
        return false;
    if (kind != NetworkPacketKind::UserMessage && kind != NetworkPacketKind::Replication)
        return false;

    try
    {
        output.payload.assign(cursor, end);
    }
    catch (const std::bad_alloc&)
    {
        output.payload.clear();
        return false;
    }
    output.kind = kind;
    output.sequence = sequence;
    return true;
}

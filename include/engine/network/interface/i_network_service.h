#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

using NetworkPeerId = uint64_t;
inline constexpr NetworkPeerId InvalidNetworkPeer = 0;

struct NetworkConfig
{
    std::string host;
    uint16_t port = 12345;
    size_t maxClients = 32;
    size_t channels = 2;
    uint32_t incomingBandwidth = 0;
    uint32_t outgoingBandwidth = 0;
};

struct NetworkStats
{
    size_t connectedPeers = 0;
    size_t peerCapacity = 0;
    uint64_t totalSentBytes = 0;
    uint64_t totalReceivedBytes = 0;
    uint32_t totalSentPackets = 0;
    uint32_t totalReceivedPackets = 0;
};

struct NetworkPeerInfo
{
    NetworkPeerId id = InvalidNetworkPeer;
    std::string address;
    uint16_t port = 0;
    uint32_t roundTripTimeMs = 0;
    float packetLossPercent = 0.0f;
    uint32_t throttle = 0;
};

class INetworkService
{
public:
    using ConnectCallback = std::function<void(NetworkPeerId)>;
    using DisconnectCallback = std::function<void(NetworkPeerId)>;
    using MessageCallback =
        std::function<void(NetworkPeerId, const uint8_t* data, size_t size, uint8_t channel)>;

    virtual ~INetworkService() = default;
    virtual bool StartServer(const NetworkConfig& config) = 0;
    virtual bool StartClient(const NetworkConfig& config) = 0;
    virtual void Stop() = 0;
    virtual void UpdateEvents(uint32_t timeoutMs = 0) = 0;
    virtual bool SendPacket(NetworkPeerId peer, const void* data, size_t size, bool reliable = true,
                            uint8_t channel = 0) = 0;
    virtual void BroadcastPacket(const void* data, size_t size, bool reliable = true, uint8_t channel = 0) = 0;
    virtual bool DisconnectPeer(NetworkPeerId peer) = 0;
    virtual void SetOnConnect(ConnectCallback callback) = 0;
    virtual void SetOnDisconnect(DisconnectCallback callback) = 0;
    virtual void SetOnMessage(MessageCallback callback) = 0;
    virtual bool IsServer() const = 0;
    virtual bool IsClient() const = 0;
    virtual bool IsRunning() const = 0;
    virtual NetworkPeerId GetServerPeer() const = 0;
    virtual NetworkPeerId GetFirstConnectedPeer() const = 0;
    virtual size_t GetConnectedPeerCount() const = 0;
    virtual std::string GetClientPeerState() const = 0;
    virtual NetworkStats GetStats() const = 0;
    virtual std::vector<NetworkPeerInfo> GetPeers() const = 0;
};

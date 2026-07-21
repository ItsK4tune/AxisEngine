#pragma once

#include <ecs/interface/i_update_system.h>
#include <network/interface/i_network_service.h>
#include <core/interface/i_optimization_configurable.h>
#include <unordered_map>

struct _ENetHost;
struct _ENetPeer;

class NetworkSystem : public IUpdateSystem, public INetworkService, public IOptimizationConfigurable
{
public:
    static bool Init();
    static void ShutdownGlobal();

    NetworkSystem();
    ~NetworkSystem() override;

    // IBaseSystem Overrides
    void Initialize() override;
    void Shutdown() override;

    bool IsEnabled() const override
    {
        return enabled;
    }
    void SetEnabled(bool enable) override
    {
        enabled = enable;
    }
    void SetPerformanceConfig(bool batchingEnabled, size_t eventsPerUpdate, float eventProcessingMs,
                              size_t bytesPerUpdate, bool replicationEnabled = true,
                              float replicationRateHz = 20.0f, float interestRadius = 0.0f)
    {
        networkBatchingEnabled = batchingEnabled;
        maxEventsPerUpdate = eventsPerUpdate > 0 ? eventsPerUpdate : 1;
        maxEventProcessingMs = eventProcessingMs > 0.0f ? eventProcessingMs : 0.01f;
        maxBytesPerUpdate = bytesPerUpdate > 0 ? bytesPerUpdate : 1;
        networkReplicationEnabled = replicationEnabled;
        networkReplicationRateHz = replicationRateHz > 0.0f ? replicationRateHz : 0.1f;
        networkInterestRadius = interestRadius > 0.0f ? interestRadius : 0.0f;
    }
    std::string GetName() const override
    {
        return "NetworkSystem";
    }
    SystemCategory GetCategory() const override
    {
        return SystemCategory::Update;
    }

    // IUpdateSystem Overrides
    void Update(Scene& scene, float dt) override;
    void ApplyOptimizationConfig(const OptimizationConfig& config) override;

    // Connection Setup
    bool StartServer(const NetworkConfig& config) override;
    bool StartClient(const NetworkConfig& config) override;
    void Stop() override;

    // Event Polling & Transmission
    void UpdateEvents(uint32_t timeoutMs = 0) override;
    bool SendPacket(NetworkPeerId peer, const void* data, size_t size, bool reliable = true,
                    uint8_t channel = 0) override;
    void BroadcastPacket(const void* data, size_t size, bool reliable = true, uint8_t channel = 0) override;
    bool DisconnectPeer(NetworkPeerId peer) override;

    // Callbacks
    void SetOnConnect(ConnectCallback callback) override
    {
        onConnect = callback;
    }
    void SetOnDisconnect(DisconnectCallback callback) override
    {
        onDisconnect = callback;
    }
    void SetOnMessage(MessageCallback callback) override
    {
        onMessage = callback;
    }

    bool IsServer() const override
    {
        return isServer;
    }
    bool IsClient() const override
    {
        return isClient;
    }
    bool IsRunning() const override
    {
        return host != nullptr;
    }
    NetworkPeerId GetServerPeer() const override;
    NetworkPeerId GetFirstConnectedPeer() const override;
    size_t GetConnectedPeerCount() const override;
    std::string GetClientPeerState() const override;
    NetworkStats GetStats() const override;
    std::vector<NetworkPeerInfo> GetPeers() const override;

private:
    _ENetPeer* FindPeer(NetworkPeerId id) const;
    static NetworkPeerId ToPeerId(const _ENetPeer* peer);
    void FlushOutgoing();
    void ReplicateScene(Scene& scene, float dt);
    bool HandleReplicationPacket(const uint8_t* data, size_t size);

    _ENetHost* host = nullptr;
    _ENetPeer* serverPeer = nullptr;
    bool isServer = false;
    bool isClient = false;
    bool enabled = true;
    bool globalInitialized = false;
    bool flushPending = false;
    bool networkBatchingEnabled = true;
    size_t maxEventsPerUpdate = 256;
    float maxEventProcessingMs = 2.0f;
    size_t maxBytesPerUpdate = 1024 * 1024;
    bool networkReplicationEnabled = true;
    float networkReplicationRateHz = 20.0f;
    float networkInterestRadius = 0.0f;
    float replicationAccumulator = 0.0f;
    Scene* currentScene = nullptr;
    std::unordered_map<NetworkPeerId, std::unordered_map<uint32_t, uint64_t>> lastReplicationSignatures;

    ConnectCallback onConnect;
    DisconnectCallback onDisconnect;
    MessageCallback onMessage;
};

#pragma once

#include <ecs/interface/i_update_system.h>
#include <enet/enet.h>
#include <functional>
#include <string>

struct NetworkConfig
{
    std::string host = "";  // Empty for server (binds to ANY), or IP address for client
    uint16_t port = 12345;
    size_t maxClients = 32;
    size_t channels = 2;
    uint32_t incomingBandwidth = 0;
    uint32_t outgoingBandwidth = 0;
};

class NetworkSystem : public IUpdateSystem
{
public:
    using ConnectCallback = std::function<void(ENetPeer* peer)>;
    using DisconnectCallback = std::function<void(ENetPeer* peer)>;
    using MessageCallback = std::function<void(ENetPeer* peer, const uint8_t* data, size_t size, uint8_t channel)>;

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

    // Connection Setup
    bool StartServer(const NetworkConfig& config);
    bool StartClient(const NetworkConfig& config);
    void Stop();

    // Event Polling & Transmission
    void UpdateEvents(uint32_t timeoutMs = 0);
    void SendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable = true, uint8_t channel = 0);
    void BroadcastPacket(const void* data, size_t size, bool reliable = true, uint8_t channel = 0);

    // Callbacks
    void SetOnConnect(ConnectCallback callback)
    {
        onConnect = callback;
    }
    void SetOnDisconnect(DisconnectCallback callback)
    {
        onDisconnect = callback;
    }
    void SetOnMessage(MessageCallback callback)
    {
        onMessage = callback;
    }

    bool IsServer() const
    {
        return isServer;
    }
    bool IsClient() const
    {
        return isClient;
    }
    bool IsRunning() const
    {
        return host != nullptr;
    }
    ENetHost* GetHost()
    {
        return host;
    }

private:
    ENetHost* host = nullptr;
    ENetPeer* serverPeer = nullptr;
    bool isServer = false;
    bool isClient = false;
    bool enabled = true;

    ConnectCallback onConnect;
    DisconnectCallback onDisconnect;
    MessageCallback onMessage;
};

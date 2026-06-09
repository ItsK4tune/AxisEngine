#pragma once

#include <ecs/interface/i_update_system.h>

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#include <enet/enet.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif

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
    bool useIPv6 = false;  // Default: IPv4
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
    bool IsIPv6() const
    {
        return m_useIPv6;
    }
    void UseIPv6(bool useIPv6)
    {
        m_useIPv6 = useIPv6;
    }
    ENetHost* GetHost()
    {
        return host;
    }
    ENetPeer* GetServerPeer()
    {
        return serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED ? serverPeer : nullptr;
    }
    ENetPeer* GetFirstConnectedPeer();
    size_t GetConnectedPeerCount() const;
    const char* GetClientPeerState() const;

private:
    ENetHost* host = nullptr;
    ENetPeer* serverPeer = nullptr;
    bool isServer = false;
    bool isClient = false;
    bool enabled = true;
    bool m_useIPv6 = false;

    ConnectCallback onConnect;
    DisconnectCallback onDisconnect;
    MessageCallback onMessage;
};

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <engine/network/network_system.h>
#include <ecs/logic/system_factory.h>
#include <core/logic/logger.h>

REGISTER_SYSTEM(NetworkSystem)

bool NetworkSystem::Init()
{
    int res = enet_initialize();
    if (res != 0)
    {
        LOGGER_ERROR("NetworkSystem") << "enet_initialize failed with code: " << res;
        return false;
    }
    LOGGER_INFO("NetworkSystem") << "enet_initialize succeeded";
    return true;
}

void NetworkSystem::ShutdownGlobal()
{
    enet_deinitialize();
    LOGGER_INFO("NetworkSystem") << "enet_deinitialize called";
}

NetworkSystem::NetworkSystem()
{
}

NetworkSystem::~NetworkSystem()
{
    Stop();
}

void NetworkSystem::Initialize()
{
    Init();
}

void NetworkSystem::Shutdown()
{
    Stop();
}

void NetworkSystem::Update(Scene& scene, float dt)
{
    if (enabled)
    {
        UpdateEvents(0);  // Poll events inside the main game update loop
    }
}

bool NetworkSystem::StartServer(const NetworkConfig& config)
{
    Stop();
    m_useIPv6 = false;
    if (config.useIPv6)
    {
        LOGGER_WARN("NetworkSystem") << "IPv6 was requested, but the linked ENet build exposes IPv4-only addresses; "
                                     << "falling back to IPv4";
    }

    LOGGER_INFO("NetworkSystem") << "Starting IPv4 server on port " << config.port
                                 << " with max clients: " << config.maxClients;

    ENetAddress address{};
    if (!config.host.empty())
    {
        if (enet_address_set_host_ip(&address, config.host.c_str()) != 0 &&
            enet_address_set_host(&address, config.host.c_str()) != 0)
        {
            LOGGER_ERROR("NetworkSystem") << "Failed to resolve bind host " << config.host;
            return false;
        }
    }
    else
    {
        address.host = ENET_HOST_ANY;
    }
    address.port = config.port;

    host = enet_host_create(&address, config.maxClients, config.channels, config.incomingBandwidth,
                            config.outgoingBandwidth);

    if (!host && !config.host.empty())
    {
        LOGGER_WARN("NetworkSystem") << "Failed to bind " << config.host << ":" << config.port
                                     << ", retrying on all local interfaces";
        ENetAddress fallbackAddress{};
        fallbackAddress.host = ENET_HOST_ANY;
        fallbackAddress.port = config.port;
        host = enet_host_create(&fallbackAddress, config.maxClients, config.channels, config.incomingBandwidth,
                                config.outgoingBandwidth);
    }
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to create ENet server host on port " << config.port;
        return false;
    }

    LOGGER_INFO("NetworkSystem") << "Server started successfully";
    char boundStr[64] = {};
    enet_address_get_host_ip(&host->address, boundStr, sizeof(boundStr));
    LOGGER_INFO("NetworkSystem") << "Server bound address: " << boundStr << ":" << host->address.port;
    isServer = true;
    isClient = false;
    return true;
}

bool NetworkSystem::StartClient(const NetworkConfig& config)
{
    Stop();
    m_useIPv6 = false;
    if (config.useIPv6)
    {
        LOGGER_WARN("NetworkSystem") << "IPv6 was requested, but the linked ENet build exposes IPv4-only addresses; "
                                     << "falling back to IPv4";
    }

    LOGGER_INFO("NetworkSystem") << "Connecting IPv4 client to " << config.host << ":" << config.port;

    host = enet_host_create(nullptr, 1, config.channels, config.incomingBandwidth, config.outgoingBandwidth);
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to create ENet client host";
        return false;
    }

    ENetAddress address{};
    const char* hostName = config.host.empty() ? "127.0.0.1" : config.host.c_str();

    if (enet_address_set_host_ip(&address, hostName) != 0 && enet_address_set_host(&address, hostName) != 0)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to resolve host " << hostName;
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }
    address.port = config.port;

    serverPeer = enet_host_connect(host, &address, config.channels, 0);
    if (!serverPeer)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to initiate ENet client connection to " << hostName << ":"
                                      << config.port;
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }

    char resolvedStr[64] = {};
    enet_address_get_host_ip(&address, resolvedStr, sizeof(resolvedStr));
    LOGGER_INFO("NetworkSystem") << "Client resolved address: " << resolvedStr << ":" << config.port;

    LOGGER_INFO("NetworkSystem") << "Client connection initiated, waiting for handshake...";
    isServer = false;
    isClient = true;
    return true;
}

void NetworkSystem::Stop()
{
    if (!host)
        return;

    if (isClient && serverPeer)
    {
        enet_peer_disconnect_now(serverPeer, 0);
        serverPeer = nullptr;
    }

    enet_host_destroy(host);
    host = nullptr;
    serverPeer = nullptr;
    isServer = false;
    isClient = false;
}

void NetworkSystem::UpdateEvents(uint32_t timeoutMs)
{
    if (!host)
        return;

    ENetEvent event;
    while (enet_host_service(host, &event, timeoutMs) > 0)
    {
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
                if (isClient)
                {
                    serverPeer = event.peer;
                }
                if (onConnect)
                {
                    onConnect(event.peer);
                }
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                if (onMessage)
                {
                    onMessage(event.peer, event.packet->data, event.packet->dataLength, event.channelID);
                }
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                if (event.peer == serverPeer)
                {
                    serverPeer = nullptr;
                }
                if (onDisconnect)
                {
                    onDisconnect(event.peer);
                }
                break;

            case ENET_EVENT_TYPE_NONE:
                break;
        }
    }
}

void NetworkSystem::SendPacket(ENetPeer* peer, const void* data, size_t size, bool reliable, uint8_t channel)
{
    if (!peer)
        return;
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (enet_peer_send(peer, channel, packet) == 0 && host)
    {
        enet_host_flush(host);
    }
}

void NetworkSystem::BroadcastPacket(const void* data, size_t size, bool reliable, uint8_t channel)
{
    if (!host)
        return;
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_host_broadcast(host, channel, packet);
    enet_host_flush(host);
}

ENetPeer* NetworkSystem::GetFirstConnectedPeer()
{
    if (!host || !host->peers)
        return nullptr;

    for (size_t i = 0; i < host->peerCount; ++i)
    {
        ENetPeer* peer = &host->peers[i];
        if (peer->state == ENET_PEER_STATE_CONNECTED)
            return peer;
    }
    return nullptr;
}

size_t NetworkSystem::GetConnectedPeerCount() const
{
    if (!host || !host->peers)
        return 0;

    size_t count = 0;
    for (size_t i = 0; i < host->peerCount; ++i)
    {
        if (host->peers[i].state == ENET_PEER_STATE_CONNECTED)
            ++count;
    }
    return count;
}

const char* NetworkSystem::GetClientPeerState() const
{
    if (!serverPeer)
        return "none";

    switch (serverPeer->state)
    {
        case ENET_PEER_STATE_DISCONNECTED: return "disconnected";
        case ENET_PEER_STATE_CONNECTING: return "connecting";
        case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT: return "ack_connect";
        case ENET_PEER_STATE_CONNECTION_PENDING: return "pending";
        case ENET_PEER_STATE_CONNECTION_SUCCEEDED: return "succeeded";
        case ENET_PEER_STATE_CONNECTED: return "connected";
        case ENET_PEER_STATE_DISCONNECT_LATER: return "disconnect_later";
        case ENET_PEER_STATE_DISCONNECTING: return "disconnecting";
        case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT: return "ack_disconnect";
        case ENET_PEER_STATE_ZOMBIE: return "zombie";
        default: return "unknown";
    }
}

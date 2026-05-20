#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ENET_IMPLEMENTATION
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

    LOGGER_INFO("NetworkSystem") << "Starting server on port " << config.port
                                 << " with max clients: " << config.maxClients;

    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = config.port;

    host = enet_host_create(&address, config.maxClients, config.channels, config.incomingBandwidth,
                            config.outgoingBandwidth);
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to create ENet server host on port " << config.port;
        return false;
    }

    LOGGER_INFO("NetworkSystem") << "Server started successfully";
    isServer = true;
    isClient = false;
    return true;
}

bool NetworkSystem::StartClient(const NetworkConfig& config)
{
    Stop();

    LOGGER_INFO("NetworkSystem") << "Connecting client to " << config.host << ":" << config.port;

    host = enet_host_create(nullptr, 1, config.channels, config.incomingBandwidth, config.outgoingBandwidth);
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to create ENet client host";
        return false;
    }

    ENetAddress address;
    enet_address_set_host(&address, config.host.c_str());
    address.port = config.port;

    serverPeer = enet_host_connect(host, &address, config.channels, 0);
    if (!serverPeer)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to initiate ENet client connection to " << config.host << ":"
                                      << config.port;
        enet_host_destroy(host);
        host = nullptr;
        return false;
    }

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
    enet_peer_send(peer, channel, packet);
}

void NetworkSystem::BroadcastPacket(const void* data, size_t size, bool reliable, uint8_t channel)
{
    if (!host)
        return;
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    enet_host_broadcast(host, channel, packet);
}

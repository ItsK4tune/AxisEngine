#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <engine/network/network_system.h>
#include <network/interface/i_network_security_provider.h>
#include <network/logic/network_protocol.h>
#include <core/logic/service_locator.h>
#include <core/type/app_config.h>
#include <ecs/logic/system_factory.h>
#include <core/logic/logger.h>
#include <ecs/unit/core_components.h>
#include <ecs/unit/network_components.h>
#include <scene/logic/scene.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-enum-enum-conversion"
#endif
#include <enet/enet.h>
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
#include <mutex>
#include <chrono>
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <limits>


namespace
{
std::mutex s_ENetLifecycleMutex;
size_t s_ENetUsers = 0;

constexpr uint32_t ReplicationMagic = 0x41585250u;  // AXRP
constexpr uint16_t ReplicationVersion = 1;
constexpr size_t MaxConfiguredPacketBytes = 16 * 1024 * 1024;
constexpr size_t MaxSecurityOverheadBytes = 64 * 1024;

void WriteU16(std::vector<uint8_t>& output, uint16_t value)
{
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteU32(std::vector<uint8_t>& output, uint32_t value)
{
    output.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    output.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    output.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    output.push_back(static_cast<uint8_t>(value & 0xFF));
}

void WriteFloat(std::vector<uint8_t>& output, float value)
{
    WriteU32(output, std::bit_cast<uint32_t>(value));
}

bool ReadU16(const uint8_t*& cursor, const uint8_t* end, uint16_t& value)
{
    if (end - cursor < 2)
        return false;
    value = static_cast<uint16_t>((static_cast<uint16_t>(cursor[0]) << 8) | cursor[1]);
    cursor += 2;
    return true;
}

bool ReadU32(const uint8_t*& cursor, const uint8_t* end, uint32_t& value)
{
    if (end - cursor < 4)
        return false;
    value = (static_cast<uint32_t>(cursor[0]) << 24) | (static_cast<uint32_t>(cursor[1]) << 16) |
            (static_cast<uint32_t>(cursor[2]) << 8) | static_cast<uint32_t>(cursor[3]);
    cursor += 4;
    return true;
}

bool ReadFloat(const uint8_t*& cursor, const uint8_t* end, float& value)
{
    uint32_t bits = 0;
    if (!ReadU32(cursor, end, bits))
        return false;
    value = std::bit_cast<float>(bits);
    return std::isfinite(value);
}

void HashFloat(uint64_t& hash, float value)
{
    hash ^= std::bit_cast<uint32_t>(value);
    hash *= 1099511628211ull;
}

uint64_t TransformSignature(uint8_t flags, const PositionComponent* position, const RotationComponent* rotation,
                            const ScaleComponent* scale)
{
    uint64_t hash = 1469598103934665603ull ^ flags;
    if (position)
        for (int i = 0; i < 3; ++i) HashFloat(hash, position->value[i]);
    if (rotation)
    {
        HashFloat(hash, rotation->value.x);
        HashFloat(hash, rotation->value.y);
        HashFloat(hash, rotation->value.z);
        HashFloat(hash, rotation->value.w);
    }
    if (scale)
        for (int i = 0; i < 3; ++i) HashFloat(hash, scale->value[i]);
    return hash;
}

struct ParsedReplicationEntry
{
    uint32_t networkId = 0;
    uint8_t flags = 0;
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f};
};

bool ParseReplicationEntries(const uint8_t*& cursor, const uint8_t* end, uint16_t count,
                             std::vector<ParsedReplicationEntry>& entries)
{
    entries.clear();
    entries.reserve(count);
    for (uint16_t entryIndex = 0; entryIndex < count; ++entryIndex)
    {
        ParsedReplicationEntry entry;
        if (!ReadU32(cursor, end, entry.networkId) || cursor == end || entry.networkId == 0)
            return false;
        entry.flags = *cursor++;
        if ((entry.flags & ~uint8_t{0x87}) != 0 || ((entry.flags & 0x80) != 0 && entry.flags != 0x80) ||
            ((entry.flags & 0x80) == 0 && (entry.flags & 0x07) == 0))
            return false;

        if ((entry.flags & 1) != 0)
            for (int axis = 0; axis < 3; ++axis)
                if (!ReadFloat(cursor, end, entry.position[axis]))
                    return false;
        if ((entry.flags & 2) != 0)
        {
            if (!ReadFloat(cursor, end, entry.rotation.x) || !ReadFloat(cursor, end, entry.rotation.y) ||
                !ReadFloat(cursor, end, entry.rotation.z) || !ReadFloat(cursor, end, entry.rotation.w))
                return false;
            const float lengthSquared = glm::dot(entry.rotation, entry.rotation);
            entry.rotation = lengthSquared > 0.000001f
                                 ? glm::normalize(entry.rotation)
                                 : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        }
        if ((entry.flags & 4) != 0)
            for (int axis = 0; axis < 3; ++axis)
                if (!ReadFloat(cursor, end, entry.scale[axis]))
                    return false;
        entries.push_back(entry);
    }
    return cursor == end;
}
}  // namespace

bool NetworkSystem::Init()
{
    std::lock_guard lock(s_ENetLifecycleMutex);
    if (s_ENetUsers == 0)
    {
        const int result = enet_initialize();
        if (result != 0)
        {
            LOGGER_ERROR("NetworkSystem") << "enet_initialize failed with code: " << result;
            return false;
        }
        LOGGER_INFO("NetworkSystem") << "enet_initialize succeeded";
    }
    ++s_ENetUsers;
    return true;
}

void NetworkSystem::ShutdownGlobal()
{
    std::lock_guard lock(s_ENetLifecycleMutex);
    if (s_ENetUsers == 0)
        return;
    --s_ENetUsers;
    if (s_ENetUsers == 0)
    {
        enet_deinitialize();
        LOGGER_INFO("NetworkSystem") << "enet_deinitialize called";
    }
}

NetworkSystem::NetworkSystem()
{
}

NetworkSystem::~NetworkSystem()
{
    Shutdown();
}

void NetworkSystem::Initialize()
{
    ServiceLocator::Instance().Register<INetworkService>(this);
    if (!globalInitialized)
        globalInitialized = Init();
}

void NetworkSystem::Shutdown()
{
    Stop();
    if (globalInitialized)
    {
        ShutdownGlobal();
        globalInitialized = false;
    }
}

void NetworkSystem::Update(Scene& scene, float dt)
{
    if (enabled)
    {
        currentScene = &scene;
        UpdateEvents(0);
        ReplicateScene(scene, dt);
        FlushOutgoing();
        currentScene = nullptr;
    }
}

bool NetworkSystem::StartServer(const NetworkConfig& config)
{
    if (config.maxClients == 0 || config.channels == 0 || config.channels > 255 || config.maxPacketBytes == 0 ||
        config.maxPacketBytes > MaxConfiguredPacketBytes)
    {
        LOGGER_ERROR("NetworkSystem") << "Server config requires maxClients > 0 and channels in [1, 255]";
        return false;
    }
    if (!globalInitialized)
    {
        globalInitialized = Init();
        if (!globalInitialized)
            return false;
    }
    Stop();
    if (!PrepareSecurity(config, true))
        return false;

    LOGGER_INFO("NetworkSystem") << "Starting IPv4 server on port " << config.port
                                 << " with max clients: " << config.maxClients;

    ENetAddress address{};
    if (!config.host.empty())
    {
        if (enet_address_set_host_ip(&address, config.host.c_str()) != 0 &&
            enet_address_set_host(&address, config.host.c_str()) != 0)
        {
            LOGGER_ERROR("NetworkSystem") << "Failed to resolve bind host " << config.host;
            if (securityProvider)
                securityProvider->Stop();
            securityProvider = nullptr;
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

    if (!host && !config.host.empty() && config.allowAnyInterfaceFallback)
    {
        LOGGER_WARN("NetworkSystem") << "Failed to bind " << config.host << ":" << config.port
                                     << "; explicit fallback enabled, retrying on all local interfaces";
        ENetAddress fallbackAddress{};
        fallbackAddress.host = ENET_HOST_ANY;
        fallbackAddress.port = config.port;
        host = enet_host_create(&fallbackAddress, config.maxClients, config.channels, config.incomingBandwidth,
                                config.outgoingBandwidth);
    }
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to bind ENet server to "
                                      << (config.host.empty() ? "all local interfaces" : config.host) << ":"
                                      << config.port;
        if (securityProvider)
            securityProvider->Stop();
        securityProvider = nullptr;
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
    if (config.channels == 0 || config.channels > 255 || config.maxPacketBytes == 0 ||
        config.maxPacketBytes > MaxConfiguredPacketBytes)
    {
        LOGGER_ERROR("NetworkSystem") << "Client config requires channels in [1, 255]";
        return false;
    }
    if (!globalInitialized)
    {
        globalInitialized = Init();
        if (!globalInitialized)
            return false;
    }
    Stop();
    if (!PrepareSecurity(config, false))
        return false;

    LOGGER_INFO("NetworkSystem") << "Connecting IPv4 client to " << config.host << ":" << config.port;

    host = enet_host_create(nullptr, 1, config.channels, config.incomingBandwidth, config.outgoingBandwidth);
    if (!host)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to create ENet client host";
        if (securityProvider)
            securityProvider->Stop();
        securityProvider = nullptr;
        return false;
    }

    ENetAddress address{};
    const char* hostName = config.host.empty() ? "127.0.0.1" : config.host.c_str();

    if (enet_address_set_host_ip(&address, hostName) != 0 && enet_address_set_host(&address, hostName) != 0)
    {
        LOGGER_ERROR("NetworkSystem") << "Failed to resolve host " << hostName;
        enet_host_destroy(host);
        host = nullptr;
        if (securityProvider)
            securityProvider->Stop();
        securityProvider = nullptr;
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
        if (securityProvider)
            securityProvider->Stop();
        securityProvider = nullptr;
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
    if (host)
    {
        FlushOutgoing();
        if (isClient && serverPeer)
            enet_peer_disconnect_now(serverPeer, 0);
        enet_host_destroy(host);
    }

    if (securityProvider)
        securityProvider->Stop();
    securityProvider = nullptr;
    host = nullptr;
    serverPeer = nullptr;
    isServer = false;
    isClient = false;
    flushPending = false;
    replicationAccumulator = 0.0f;
    lastReplicationSignatures.clear();
    receivedSequences.clear();
    nextSequence = 1;
    currentScene = nullptr;
}

bool NetworkSystem::PrepareSecurity(const NetworkConfig& config, bool server)
{
    securityMode = config.securityMode;
    maxPacketBytes = config.maxPacketBytes;
    if (securityMode == NetworkSecurityMode::TrustedNetwork)
    {
        LOGGER_WARN("NetworkSystem") << "Starting unauthenticated, unencrypted TrustedNetwork session";
        securityProvider = nullptr;
        return true;
    }

    securityProvider = ServiceLocator::Instance().Resolve<INetworkSecurityProvider>();
    if (!securityProvider)
    {
        LOGGER_ERROR("NetworkSystem") << "Secure networking is required but no INetworkSecurityProvider is registered";
        return false;
    }
    if (!securityProvider->Start(server, config))
    {
        LOGGER_ERROR("NetworkSystem") << "Network security provider failed to initialize";
        securityProvider->Stop();
        securityProvider = nullptr;
        return false;
    }
    return true;
}

void NetworkSystem::ApplyOptimizationConfig(const OptimizationConfig& config)
{
    SetPerformanceConfig(config.networkBatchingEnabled, static_cast<size_t>(config.networkMaxEventsPerUpdate),
                         config.networkMaxEventProcessingMs,
                         static_cast<size_t>(config.networkMaxBytesPerUpdate), config.networkReplicationEnabled,
                         config.networkReplicationRateHz, config.networkInterestRadius);
}

void NetworkSystem::UpdateEvents(uint32_t timeoutMs)
{
    if (!host)
        return;

    ENetEvent event;
    const auto started = std::chrono::steady_clock::now();
    size_t processed = 0;
    size_t processedBytes = 0;
    uint32_t serviceTimeout = timeoutMs;
    while (processed < maxEventsPerUpdate && enet_host_service(host, &event, serviceTimeout) > 0)
    {
        ++processed;
        serviceTimeout = 0;
        switch (event.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                const NetworkPeerId peerId = ToPeerId(event.peer);
                if (securityProvider &&
                    (!securityProvider->AcceptPeer(peerId) || !securityProvider->IsAuthenticated(peerId)))
                {
                    LOGGER_WARN("NetworkSystem") << "Security provider rejected or did not authenticate peer";
                    securityProvider->RemovePeer(peerId);
                    enet_peer_disconnect_now(event.peer, 1);
                    break;
                }
                if (isClient)
                {
                    serverPeer = event.peer;
                }
                if (onConnect)
                {
                    onConnect(peerId);
                }
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                processedBytes += event.packet ? event.packet->dataLength : 0;
                if (!event.packet)
                    break;

                const NetworkPeerId peerId = ToPeerId(event.peer);
                const uint8_t* wireData = event.packet->data;
                size_t wireSize = event.packet->dataLength;
                const size_t wireLimit =
                    maxPacketBytes + (securityProvider ? MaxSecurityOverheadBytes : NetworkProtocol::HeaderBytes);
                if (wireSize > wireLimit)
                {
                    LOGGER_WARN("NetworkSystem") << "Discarded oversized network packet";
                    enet_packet_destroy(event.packet);
                    break;
                }
                std::vector<uint8_t> plaintext;
                if (securityProvider)
                {
                    if (!securityProvider->IsAuthenticated(peerId) ||
                        !securityProvider->Open(peerId, wireData, wireSize, plaintext))
                    {
                        LOGGER_WARN("NetworkSystem") << "Discarded unauthenticated or invalid encrypted packet";
                        enet_packet_destroy(event.packet);
                        break;
                    }
                    wireData = plaintext.data();
                    wireSize = plaintext.size();
                }

                NetworkProtocolPacket decoded;
                if (!NetworkProtocol::Decode(wireData, wireSize, maxPacketBytes, decoded))
                {
                    LOGGER_WARN("NetworkSystem") << "Discarded malformed network protocol packet";
                    enet_packet_destroy(event.packet);
                    break;
                }

                uint32_t& lastSequence = receivedSequences[peerId][event.channelID];
                if (decoded.sequence <= lastSequence)
                {
                    LOGGER_WARN("NetworkSystem") << "Discarded replayed or out-of-order network packet";
                    enet_packet_destroy(event.packet);
                    break;
                }
                if (securityProvider &&
                    !securityProvider->AuthorizePacket(peerId, static_cast<uint8_t>(decoded.kind), event.channelID,
                                                       decoded.payload.data(), decoded.payload.size()))
                {
                    LOGGER_WARN("NetworkSystem") << "Security provider denied network packet";
                    enet_packet_destroy(event.packet);
                    break;
                }
                lastSequence = decoded.sequence;

                if (decoded.kind == NetworkPacketKind::Replication)
                    HandleReplicationPacket(decoded.payload.data(), decoded.payload.size());
                else if (onMessage)
                    onMessage(peerId, decoded.payload.data(), decoded.payload.size(), event.channelID);
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                const NetworkPeerId peerId = ToPeerId(event.peer);
                if (event.peer == serverPeer)
                {
                    serverPeer = nullptr;
                }
                if (securityProvider)
                    securityProvider->RemovePeer(peerId);
                if (onDisconnect)
                {
                    onDisconnect(peerId);
                }
                lastReplicationSignatures.erase(peerId);
                receivedSequences.erase(peerId);
                break;
            }

            case ENET_EVENT_TYPE_NONE:
                break;
        }

        const float elapsedMs =
            std::chrono::duration<float, std::milli>(std::chrono::steady_clock::now() - started).count();
        if (elapsedMs >= maxEventProcessingMs)
            break;
        if (processedBytes >= maxBytesPerUpdate)
            break;
    }
    // Calls made outside the engine frame still need to make progress. During
    // Update(), currentScene stays non-null and the single end-of-frame flush
    // batches event callbacks and replication into one ENet submission.
    if (!currentScene)
        FlushOutgoing();
}

void NetworkSystem::ReplicateScene(Scene& scene, float dt)
{
    if (!networkReplicationEnabled || !isServer || !host || networkReplicationRateHz <= 0.0f)
        return;
    replicationAccumulator += (std::max)(0.0f, dt);
    const float interval = 1.0f / networkReplicationRateHz;
    if (replicationAccumulator < interval)
        return;
    replicationAccumulator = std::fmod(replicationAccumulator, interval);

    auto replicated = scene.View<NetworkComponent>();
    for (size_t peerIndex = 0; peerIndex < host->peerCount; ++peerIndex)
    {
        ENetPeer* peer = &host->peers[peerIndex];
        if (peer->state != ENET_PEER_STATE_CONNECTED)
            continue;

        const NetworkPeerId peerId = ToPeerId(peer);
        auto& previousSignatures = lastReplicationSignatures[peerId];
        glm::vec3 observerPosition(0.0f);
        bool hasObserver = false;
        auto observers = scene.View<NetworkComponent, PositionComponent>();
        const uint32_t peerOwnerId = peer->incomingPeerID + 1;
        for (entt::entity observer : observers)
        {
            const auto& network = observers.get<NetworkComponent>(observer);
            if (network.ownerId == peerOwnerId)
            {
                observerPosition = observers.get<PositionComponent>(observer).value;
                hasObserver = true;
                break;
            }
        }

        std::vector<uint8_t> packet;
        packet.reserve((std::min)(maxBytesPerUpdate, static_cast<size_t>(64 * 1024)));
        WriteU32(packet, ReplicationMagic);
        WriteU16(packet, ReplicationVersion);
        WriteU16(packet, 0);
        if (packet.size() > maxBytesPerUpdate)
            continue;

        uint16_t count = 0;
        std::vector<std::pair<uint32_t, uint64_t>> committedUpdates;
        std::vector<uint32_t> committedRemovals;
        std::unordered_map<uint32_t, bool> visibleIds;
        visibleIds.reserve(replicated.size());
        bool packetFull = false;

        for (entt::entity entity : replicated)
        {
            const auto& network = replicated.get<NetworkComponent>(entity);
            if (network.networkId == 0 || !network.replicateTransform)
                continue;

            const auto* position = scene.TryGetComponent<PositionComponent>(entity);
            const auto* rotation = scene.TryGetComponent<RotationComponent>(entity);
            const auto* scale = scene.TryGetComponent<ScaleComponent>(entity);
            uint8_t flags = 0;
            if (position) flags |= 1;
            if (rotation) flags |= 2;
            if (scale) flags |= 4;
            if (flags == 0)
                continue;

            const float interestRadius = network.interestRadius > 0.0f ? network.interestRadius : networkInterestRadius;
            if (interestRadius > 0.0f && hasObserver && position)
            {
                const glm::vec3 delta = position->value - observerPosition;
                if (glm::dot(delta, delta) > interestRadius * interestRadius)
                    continue;
            }
            visibleIds[network.networkId] = true;

            const uint64_t signature = TransformSignature(flags, position, rotation, scale);
            const auto previous = previousSignatures.find(network.networkId);
            if (previous != previousSignatures.end() && previous->second == signature)
                continue;
            if (packetFull)
                continue;

            const size_t entryBytes = 5 + (position ? 12 : 0) + (rotation ? 16 : 0) + (scale ? 12 : 0);
            if (count == (std::numeric_limits<uint16_t>::max)() || packet.size() + entryBytes > maxBytesPerUpdate)
            {
                packetFull = true;
                continue;
            }
            WriteU32(packet, network.networkId);
            packet.push_back(flags);
            if (position)
                for (int i = 0; i < 3; ++i) WriteFloat(packet, position->value[i]);
            if (rotation)
            {
                WriteFloat(packet, rotation->value.x);
                WriteFloat(packet, rotation->value.y);
                WriteFloat(packet, rotation->value.z);
                WriteFloat(packet, rotation->value.w);
            }
            if (scale)
                for (int i = 0; i < 3; ++i) WriteFloat(packet, scale->value[i]);
            committedUpdates.emplace_back(network.networkId, signature);
            ++count;
        }

        for (const auto& [networkId, signature] : previousSignatures)
        {
            (void)signature;
            if (visibleIds.contains(networkId))
                continue;
            if (count == (std::numeric_limits<uint16_t>::max)() || packet.size() + 5 > maxBytesPerUpdate)
                break;
            WriteU32(packet, networkId);
            packet.push_back(0x80);
            committedRemovals.push_back(networkId);
            ++count;
        }

        if (count == 0)
            continue;
        packet[6] = static_cast<uint8_t>((count >> 8) & 0xFF);
        packet[7] = static_cast<uint8_t>(count & 0xFF);
        if (!SendProtocolPacket(peerId, NetworkPacketKind::Replication, packet.data(), packet.size(), true, 0))
            continue;
        for (const auto& [networkId, signature] : committedUpdates)
            previousSignatures[networkId] = signature;
        for (uint32_t networkId : committedRemovals)
            previousSignatures.erase(networkId);
    }
}

bool NetworkSystem::HandleReplicationPacket(const uint8_t* data, size_t size)
{
    if (!data || size < 8)
        return false;
    const uint8_t* cursor = data;
    const uint8_t* end = data + size;
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t count = 0;
    if (!ReadU32(cursor, end, magic) || magic != ReplicationMagic)
        return false;
    if (!ReadU16(cursor, end, version) || !ReadU16(cursor, end, count) || version != ReplicationVersion)
        return true;
    if (!currentScene || !isClient)
        return true;

    std::vector<ParsedReplicationEntry> entries;
    if (!ParseReplicationEntries(cursor, end, count, entries))
    {
        LOGGER_WARN("NetworkSystem") << "Discarded malformed replication packet";
        return true;
    }

    // Parsing is deliberately completed before touching the scene. A truncated
    // or non-finite packet therefore cannot leave a partially-applied snapshot.
    auto& scene = *currentScene;
    std::unordered_map<uint32_t, entt::entity> entitiesByNetworkId;
    auto existing = scene.View<NetworkComponent>();
    entitiesByNetworkId.reserve(existing.size());
    for (entt::entity entity : existing)
    {
        const uint32_t networkId = existing.get<NetworkComponent>(entity).networkId;
        if (networkId != 0)
            entitiesByNetworkId[networkId] = entity;
    }

    for (const ParsedReplicationEntry& entry : entries)
    {
        auto found = entitiesByNetworkId.find(entry.networkId);
        if ((entry.flags & 0x80) != 0)
        {
            if (found != entitiesByNetworkId.end())
            {
                const auto& network = scene.GetComponent<NetworkComponent>(found->second);
                if (!network.isLocal)
                    scene.Destroy(found->second);
            }
            continue;
        }

        entt::entity entity = entt::null;
        if (found == entitiesByNetworkId.end())
        {
            entity = scene.CreateEntityWithTransform("Network_" + std::to_string(entry.networkId), entry.position)
                         .GetHandle();
            auto& network = scene.AddComponent<NetworkComponent>(entity);
            network.networkId = entry.networkId;
            network.replicateTransform = true;
            entitiesByNetworkId[entry.networkId] = entity;
        }
        else
        {
            entity = found->second;
            if (scene.GetComponent<NetworkComponent>(entity).isLocal)
                continue;
        }

        if ((entry.flags & 1) != 0)
        {
            auto& component = scene.GetOrAddComponent<PositionComponent>(entity);
            component.prev = component.value;
            component.value = entry.position;
        }
        if ((entry.flags & 2) != 0)
        {
            auto& component = scene.GetOrAddComponent<RotationComponent>(entity);
            component.prev = component.value;
            component.value = entry.rotation;
        }
        if ((entry.flags & 4) != 0)
        {
            auto& component = scene.GetOrAddComponent<ScaleComponent>(entity);
            component.prev = component.value;
            component.value = entry.scale;
        }
        scene.GetOrAddComponent<WorldTransformComponent>(entity).isDirty = true;
        scene.MarkTransformDirty(entity);
    }
    return true;
}

void NetworkSystem::FlushOutgoing()
{
    if (host && flushPending)
    {
        enet_host_flush(host);
        flushPending = false;
    }
}

bool NetworkSystem::SendPacket(NetworkPeerId peerId, const void* data, size_t size, bool reliable, uint8_t channel)
{
    return SendProtocolPacket(peerId, NetworkPacketKind::UserMessage, data, size, reliable, channel);
}

bool NetworkSystem::SendProtocolPacket(NetworkPeerId peerId, NetworkPacketKind kind, const void* data, size_t size,
                                       bool reliable, uint8_t channel)
{
    ENetPeer* peer = FindPeer(peerId);
    if (!peer || peer->state != ENET_PEER_STATE_CONNECTED || channel >= peer->channelCount ||
        (size != 0 && !data))
        return false;

    if (nextSequence == 0)
    {
        LOGGER_ERROR("NetworkSystem") << "Network packet sequence exhausted; restart the session";
        return false;
    }
    const uint32_t sequence = nextSequence++;

    std::vector<uint8_t> encoded;
    if (!NetworkProtocol::Encode(kind, sequence, data, size, maxPacketBytes, encoded))
        return false;

    if (securityProvider)
    {
        if (!securityProvider->IsAuthenticated(peerId))
            return false;
        std::vector<uint8_t> ciphertext;
        if (!securityProvider->Seal(peerId, encoded.data(), encoded.size(), ciphertext) || ciphertext.empty() ||
            ciphertext.size() > maxPacketBytes + MaxSecurityOverheadBytes)
            return false;
        return SendWirePacket(peer, ciphertext.data(), ciphertext.size(), reliable, channel);
    }
    return SendWirePacket(peer, encoded.data(), encoded.size(), reliable, channel);
}

bool NetworkSystem::SendWirePacket(ENetPeer* peer, const void* data, size_t size, bool reliable, uint8_t channel)
{
    if (!peer || peer->state != ENET_PEER_STATE_CONNECTED || channel >= peer->channelCount ||
        (size != 0 && !data))
        return false;
    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;
    ENetPacket* packet = enet_packet_create(data, size, flags);
    if (!packet)
        return false;
    if (enet_peer_send(peer, channel, packet) == 0)
    {
        flushPending = true;
        if (!networkBatchingEnabled)
            FlushOutgoing();
        return true;
    }
    enet_packet_destroy(packet);
    return false;
}

void NetworkSystem::BroadcastPacket(const void* data, size_t size, bool reliable, uint8_t channel)
{
    if (!host || channel >= host->channelLimit || (size != 0 && !data))
        return;
    for (size_t peerIndex = 0; peerIndex < host->peerCount; ++peerIndex)
    {
        ENetPeer* peer = &host->peers[peerIndex];
        if (peer->state == ENET_PEER_STATE_CONNECTED)
            SendProtocolPacket(ToPeerId(peer), NetworkPacketKind::UserMessage, data, size, reliable, channel);
    }
}

NetworkPeerId NetworkSystem::GetFirstConnectedPeer() const
{
    if (!host || !host->peers)
        return InvalidNetworkPeer;

    for (size_t i = 0; i < host->peerCount; ++i)
    {
        ENetPeer* peer = &host->peers[i];
        if (peer->state == ENET_PEER_STATE_CONNECTED)
            return ToPeerId(peer);
    }
    return InvalidNetworkPeer;
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

std::string NetworkSystem::GetClientPeerState() const
{
    if (!serverPeer)
        return "none";

    switch (serverPeer->state)
    {
        case ENET_PEER_STATE_DISCONNECTED:
            return "disconnected";
        case ENET_PEER_STATE_CONNECTING:
            return "connecting";
        case ENET_PEER_STATE_ACKNOWLEDGING_CONNECT:
            return "ack_connect";
        case ENET_PEER_STATE_CONNECTION_PENDING:
            return "pending";
        case ENET_PEER_STATE_CONNECTION_SUCCEEDED:
            return "succeeded";
        case ENET_PEER_STATE_CONNECTED:
            return "connected";
        case ENET_PEER_STATE_DISCONNECT_LATER:
            return "disconnect_later";
        case ENET_PEER_STATE_DISCONNECTING:
            return "disconnecting";
        case ENET_PEER_STATE_ACKNOWLEDGING_DISCONNECT:
            return "ack_disconnect";
        case ENET_PEER_STATE_ZOMBIE:
            return "zombie";
        default:
            return "unknown";
    }
}

NetworkPeerId NetworkSystem::GetServerPeer() const
{
    return serverPeer && serverPeer->state == ENET_PEER_STATE_CONNECTED ? ToPeerId(serverPeer) : InvalidNetworkPeer;
}

NetworkPeerId NetworkSystem::ToPeerId(const _ENetPeer* peer)
{
    return peer ? static_cast<NetworkPeerId>(reinterpret_cast<uintptr_t>(peer)) : InvalidNetworkPeer;
}

_ENetPeer* NetworkSystem::FindPeer(NetworkPeerId id) const
{
    if (!host || !host->peers || id == InvalidNetworkPeer)
        return nullptr;
    const uintptr_t candidateAddress = static_cast<uintptr_t>(id);
    const uintptr_t beginAddress = reinterpret_cast<uintptr_t>(host->peers);
    const uintptr_t endAddress = beginAddress + host->peerCount * sizeof(ENetPeer);
    if (candidateAddress < beginAddress || candidateAddress >= endAddress ||
        (candidateAddress - beginAddress) % sizeof(ENetPeer) != 0)
        return nullptr;
    return reinterpret_cast<ENetPeer*>(candidateAddress);
}

bool NetworkSystem::DisconnectPeer(NetworkPeerId peer)
{
    if (auto* nativePeer = FindPeer(peer))
    {
        enet_peer_disconnect(nativePeer, 0);
        return true;
    }
    return false;
}

NetworkStats NetworkSystem::GetStats() const
{
    if (!host)
        return {};
    return {host->connectedPeers, host->peerCount, host->totalSentData, host->totalReceivedData,
            host->totalSentPackets, host->totalReceivedPackets};
}

std::vector<NetworkPeerInfo> NetworkSystem::GetPeers() const
{
    std::vector<NetworkPeerInfo> peers;
    if (!host || !host->peers)
        return peers;
    peers.reserve(host->connectedPeers);
    for (size_t index = 0; index < host->peerCount; ++index)
    {
        const ENetPeer* peer = &host->peers[index];
        if (peer->state != ENET_PEER_STATE_CONNECTED)
            continue;
        char address[64] = {};
        enet_address_get_host_ip(&peer->address, address, sizeof(address));
        peers.push_back({ToPeerId(peer), address, peer->address.port, peer->roundTripTime,
                         static_cast<float>(peer->packetLoss) * 100.0f / 65536.0f, peer->packetThrottle});
    }
    return peers;
}

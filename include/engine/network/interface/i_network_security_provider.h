#pragma once

#include <network/interface/i_network_service.h>
#include <cstddef>
#include <cstdint>
#include <vector>

class INetworkSecurityProvider
{
public:
    virtual ~INetworkSecurityProvider() = default;
    virtual bool Start(bool server, const NetworkConfig& config) = 0;
    virtual void Stop() = 0;
    virtual bool AcceptPeer(NetworkPeerId peer) = 0;
    virtual void RemovePeer(NetworkPeerId peer) = 0;
    virtual bool IsAuthenticated(NetworkPeerId peer) const = 0;
    virtual bool Seal(NetworkPeerId peer, const uint8_t* plaintext, size_t size,
                      std::vector<uint8_t>& ciphertext) = 0;
    virtual bool Open(NetworkPeerId peer, const uint8_t* ciphertext, size_t size,
                      std::vector<uint8_t>& plaintext) = 0;
    virtual bool AuthorizePacket(NetworkPeerId peer, uint8_t packetKind, uint8_t channel,
                                 const uint8_t* payload, size_t size) const = 0;
};

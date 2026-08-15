# Extending the Engine (Plugins & Providers)

> [Tiếng Việt](../../vi/guides/extending_engine.md) | [Public API Surface](../core/api_surface.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine uses an abstract strategy architecture to allow developers to build custom pluggable modules, providers, and system plugins using `#include <axis_plugin.h>`.

---

## 2. How to Use

1. **Include Plugin Header**: `#include <axis_plugin.h>`.
2. **Subclass Provider Contract**: Subclass `IGraphicsContext`, `IPhysicsWorld`, `IAudioService`, or `INetworkSecurityProvider`.
3. **Register Instance**: Register provider instance into `ServiceLocator`.

---

## 3. Examples

### Custom Network Security Provider Example
```cpp
#include <axis_plugin.h>

class CustomSecurityProvider final : public INetworkSecurityProvider {
public:
    bool AuthenticateHandshake(const NetworkPeerInfo& peer) override { return true; }
    bool SealPacket(const uint8_t* in, size_t size, std::vector<uint8_t>& out) override {
        out.assign(in, in + size);
        return true;
    }
    bool OpenPacket(const uint8_t* in, size_t size, std::vector<uint8_t>& out) override {
        out.assign(in, in + size);
        return true;
    }
};

void RegisterCustomSecurity() {
    ServiceLocator::Register<INetworkSecurityProvider>(
        std::make_shared<CustomSecurityProvider>());
}
```

---

## 4. API & Configuration Reference

### Provider Interface Contracts Reference

| Provider Contract | Header Path | Extension Purpose |
| :--- | :--- | :--- |
| `IGraphicsContext` | `<engine/graphics/igraphics_context.h>` | Custom rendering pipeline provider |
| `IPhysicsWorld` | `<engine/physics/iphysics_world.h>` | Custom 3D/2D physics simulation provider |
| `IAudioService` | `<engine/audio/iaudio_service.h>` | Custom audio playback backend provider |
| `INetworkSecurityProvider` | `<engine/network/isecurity_provider.h>` | Custom network encryption & authentication provider |

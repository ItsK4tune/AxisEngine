# Hướng dẫn Mở rộng Engine (Plugins & Providers)

> [English](../../eng/guides/extending_engine.md) | [Bề mặt API Công khai](../core/api_surface.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine sử dụng kiến trúc strategy trừu tượng cho phép các nhà phát triển xây dựng các mô-đun cắm rút tùy chỉnh, provider và plugin hệ thống bằng cách sử dụng `#include <axis_plugin.h>`.

---

## 2. Cách dùng

1. **Include Plugin Header**: `#include <axis_plugin.h>`.
2. **Kế thừa Hợp đồng Provider**: Kế thừa `IGraphicsContext`, `IPhysicsWorld`, `IAudioService`, hoặc `INetworkSecurityProvider`.
3. **Đăng ký Instance**: Đăng ký instance provider vào `ServiceLocator`.

---

## 3. Ví dụ

### Ví dụ Provider Bảo mật Mạng Tùy chỉnh
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

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Hợp đồng Giao diện Provider

| Hợp đồng Provider | Đường dẫn Header | Mục đích Mở rộng |
| :--- | :--- | :--- |
| `IGraphicsContext` | `<engine/graphics/igraphics_context.h>` | Provider pipeline dựng hình đồ họa tùy chỉnh |
| `IPhysicsWorld` | `<engine/physics/iphysics_world.h>` | Provider mô phỏng vật lý 3D/2D tùy chỉnh |
| `IAudioService` | `<engine/audio/iaudio_service.h>` | Provider backend phát âm thanh tùy chỉnh |
| `INetworkSecurityProvider` | `<engine/network/isecurity_provider.h>` | Provider mã hóa và xác thực mạng tùy chỉnh |

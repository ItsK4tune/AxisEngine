# Kiến trúc module lai

> [English](../../eng/core/architecture.md)

## 1. Triết lý thiết kế

AxisEngine kết hợp ECS cho dữ liệu gameplay, interface/provider cho backend và
service context cho lifecycle ứng dụng. Mục tiêu là giữ gameplay độc lập với
OpenGL, Bullet, audio backend và editor cụ thể.

## 2. Các lớp cấu trúc

1. **Public contracts** trong `axis_sdk.h` và `axis_plugin.h`.
2. **Logic** gồm system, manager, serializer và registry.
3. **Strategy/backend** triển khai graphics, physics, audio và platform.
4. **Application host** tạo provider, system catalog, state machine và vòng lặp.
5. **Editor/sample** là consumer của engine, không phải lõi bắt buộc.

Interface được đăng ký theo type trong application service context. Module có
thể thay system hoặc provider qua registry/factory trước khi application tạo
system mặc định.

## 3. Mô hình thực thi và bộ nhớ

- Main thread điều phối state, event, ECS update và render.
- Job System xử lý công việc song song; task trước initialize chạy inline để
  không tạo future bị bỏ quên.
- Snapshot cấu hình và service registry dùng copy-on-write khi cần đọc đồng thời.
- GPU/audio/physics object tuân theo RAII và shutdown theo thứ tự ngược initialize.

## 4. Entity–Component–System

### Entity

Entity là ID nhẹ của EnTT, không tự chứa dữ liệu. Dùng
[`EntityBuilder`](../scripting/scriptable_api.md#entitybuilder) khi cần tạo theo
fluent API.

### Component

Component chỉ chứa dữ liệu authoring/runtime. Dữ liệu transient như GPU handle,
world matrix suy ra, contact state và playback state không được serialize.

### System

System xử lý view component theo category và lifecycle. `SystemManager` tạo
catalog, áp capability/config và shutdown system đã initialize theo thứ tự ngược.

## 5. Logic chính

- **Rendering:** scene → culling → render queue → shadow/geometry/lighting →
  transparent/UI/post-process.
- **Physics:** component shape/body được binding sang Bullet sau scene load;
  transform sync theo fixed update.
- **Asset:** `ResourceManager` điều phối cache, loader, async publication và hot reload.
- **Scene:** YAML `.axs` hoặc binary `.axsb` → component loader → validation →
  post-load fixup.
- **Network:** ENet transport, protocol envelope và security provider do ứng dụng cấp.

## 6. Nguyên tắc bộ nhớ

- Ưu tiên value type và smart pointer; raw pointer chỉ là reference không sở hữu.
- Resource dùng identity ổn định để async load không thay object mà consumer giữ.
- Không giữ reference component qua thao tác có thể làm registry reallocate.
- Module phải unregister callback/registry entry trước khi unload code.

## Xem thêm

- [Scriptable API](../scripting/scriptable_api.md)
- [Scene format](../guides/scene_format.md)
- [Cấu trúc project](../guides/project_structure.md)

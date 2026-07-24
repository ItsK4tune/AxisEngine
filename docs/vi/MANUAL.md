# Manual AxisEngine

> [English](../eng/MANUAL.md)

Tài liệu này mô tả repository tại commit `787f438` ngày 2026-07-23. Khi tài
liệu và code khác nhau, public header, CMake, sample và test là nguồn chuẩn.

## 1. Phạm vi sản phẩm

AxisEngine là static library C++20, kèm static editor library tùy chọn. Engine
phù hợp để xây game native, simulation, visualization tool và thử nghiệm engine.
Hiện tại đây không phải sandbox cho asset không tin cậy hay network service
hoàn chỉnh để public trực tiếp lên Internet.

Runtime gồm:

1. `Application` sở hữu provider và lifecycle;
2. `StateMachine` dạng stack;
3. `Scene` dùng EnTT registry;
4. system đăng ký qua `SystemFactory`/`ISystemRegistry`;
5. service được activate qua `ServiceLocator` của từng application;
6. scene `.axs` và binary `.axsb`.

Mỗi process chỉ có một `Application` được initialize và active tại một thời
điểm. Event manager, job system, logger, profiler và cầu nối service vẫn mang
tính process-global.

## 2. Backend hiện có

| Chức năng | Backend | Cách chọn |
| --- | --- | --- |
| Graphics | OpenGL | `AXIS_GRAPHICS_BACKEND=OpenGL` |
| Physics | Bullet | `AXIS_PHYSICS_BACKEND=Bullet` |
| Audio playback | Null | Mặc định |
| Audio playback | FMOD | `AXIS_AUDIO_BACKEND=FMOD` và FMOD SDK |
| Audio playback | irrKlang | `AXIS_AUDIO_BACKEND=IrrKlang` và irrKlang SDK |
| Microphone | WASAPI | Windows |
| Microphone | Unsupported service | Nền tảng khác |
| Window/input | GLFW | Đi cùng OpenGL provider |

Vulkan, DirectX, PhysX và OpenAL chưa được implement. macOS có source và preset
build, nhưng renderer yêu cầu OpenGL 4.6 trong khi macOS chỉ có OpenGL 4.1; vì
vậy không xem macOS là runtime rendering target được hỗ trợ.

## 3. Build

Build engine mặc định:

```powershell
cmake --preset windows-msvc
cmake --build --preset windows-release
```

Build editor và sample:

```powershell
cmake --preset windows-msvc-editor
cmake --build build --config Release --parallel
.\build\bin\Release\axis_samples.exe
```

Build test:

```powershell
cmake -S . -B build -DENABLE_TESTS=ON
cmake --build build --config Release --target axis_test --parallel
ctest --test-dir build -C Release --output-on-failure
```

Các option chính:

| Option | Mặc định | Ý nghĩa |
| --- | --- | --- |
| `ENABLE_EDITOR` | `OFF` | Build `axis_editor` |
| `BUILD_SAMPLES` | `ON` | Build `axis_samples` nếu editor được bật |
| `ENABLE_TESTS` | `OFF` | Build `axis_test` |
| `ENABLE_LTO` | `ON` | IPO/LTO cho optimized build |
| `ENABLE_PCH` | `ON` | Precompiled header |
| `ENABLE_UNITY_BUILD` | `ON` | Unity build cho translation unit an toàn |

Target `axis_compile` không nằm trong build mặc định. Build rõ ràng bằng:

```powershell
cmake --build build --config Release --target axis_compile
.\build\bin\Release\axis_compile.exe input.axs output.axsb
```

## 4. Public API

- `axis_sdk.h`: dùng cho game và tool.
- `axis_plugin.h`: contract để thay provider/module.
- `axis_advanced.h`: system cấp thấp, chi phí nâng version cao hơn.
- `axis_all.h`: tiện khi làm việc trong source tree; consumer nên dùng header
  hẹp hơn.
- `engine/**/strategy/**`: implementation detail, không được install.

CMake package export `Axis::Engine` và `Axis::Editor` khi editor được build.
Asset mặc định được cài vào `share/AxisEngine/assets`.

## 5. Application, state và script

Ứng dụng kế thừa `Application`, đăng ký script/state/system nếu cần, initialize
`AppConfig`, push state đầu tiên rồi gọi `Run`.

```cpp
#include <axis_sdk.h>

class GameState final : public State
{
public:
    void OnEnter() override {}
    void OnUpdate(float) override {}
    void OnRender() override {}
    void OnExit() override {}
};

class GameApplication final : public Application
{
public:
    void RegisterUserScripts() override
    {
        // RegisterScript<PlayerController>("PlayerController");
    }
};
```

Dùng `State` cho mode toàn cục như boot, menu, gameplay, pause. Dùng
`Scriptable` cho behavior của một entity. Dùng ECS system cho logic chạy trên
nhiều entity hoặc cần update/render phase cụ thể.

Khi sửa trực tiếp transform component, gọi `MarkTransformDirty`; tốt hơn là dùng
API của `Scene`, `Entity` hoặc `EntityBuilder`.

## 6. Scene và asset

`.axs` là format indentation-based nhỏ do `YAMLParser` riêng xử lý, không phải
YAML đầy đủ. Dùng space để indent; tab indentation bị từ chối kèm line/column.
Tránh anchor, alias, flow collection, multiline scalar và cú pháp YAML nâng cao.

`.axsb` là scene binary có version. Bản hiện tại chứa payload `.axs` đã
serialize. Loader giới hạn file, payload, string và entity theo cấu hình; legacy
load rollback thay đổi scene khi thất bại. Xem cả scene lẫn asset được tham
chiếu là dữ liệu tin cậy của project.

Resource có thể khai báo trong `Resources:` hoặc load qua `ResourceManager`.
Texture có thể decode bất đồng bộ; GPU upload và event hoàn tất diễn ra trong
manager update. Tên resource là alias và manager deduplicate theo physical path.

`asset://...` dùng cho asset nội bộ của engine. Asset game thường dùng path
tương đối theo project root. Absolute path vẫn được chấp nhận, nên hệ thống path
không phải security boundary.

## 7. Runtime

- Renderer: forward/deferred, PBR, light/shadow, post-process, decal, terrain,
  particle, reflection probe, planar reflection, UI, video texture, culling và
  batching.
- Physics: rigid body, shape, constraint, ray query, character controller,
  collision filtering và transform sync qua Bullet.
- Navigation: navmesh generation, dirty-region rebuild, grid pathfinding, path
  following, tag và custom cost rule.
- Audio: playback 2D/3D qua backend đã chọn; microphone là service riêng.
- Network: ENet client/server messaging và transform replication, kèm protocol
  envelope kiểm tra packet kind, size và sequence. Secure mode cần
  `INetworkSecurityProvider` do ứng dụng cung cấp.

## 8. Editor

Build với `ENABLE_EDITOR=ON` và link `Axis::Editor`. `axis_samples` là host tham
chiếu. Editor có hierarchy/inspector, project assets, file browser, resource
preview, prefab, input actions, lighting/lightmap, navigation, network,
profiler, frame debugger, animation/VFX graph, console, settings và
play/edit/stop.

Editor thao tác trực tiếp lên file project. Luôn dùng version control. File
Hierarchy giới hạn trong canonical project root, create-exclusive, sinh tên
duplicate duy nhất và không rename đè file có sẵn. Các lệnh save/apply được chọn
rõ vẫn có thể thay output của chính chúng.

## 9. Network và an toàn

ENet cung cấp UDP channel reliable/unreliable, không tự cung cấp bảo mật ứng
dụng. `NetworkConfig` mặc định `RequireSecure`; startup thất bại nếu
`INetworkSecurityProvider` chưa sẵn sàng để authenticate peer, seal/open packet
và authorize message. AxisEngine chưa đi kèm cryptographic provider.
`TrustedNetwork` bỏ qua bảo vệ này một cách tường minh và phát cảnh báo.

- Bind vào interface rõ ràng và kiểm tra địa chỉ thực tế trong log.
- Editor mặc định trường host/bind là `127.0.0.1`; xóa trống trường này là yêu
  cầu tường minh để bind mọi interface cục bộ.
- Đăng ký security provider đã được review trước khi public ra Internet.
- Validate message type, size, rate, identity và permission ở tầng game.
- Không xem replicated transform là authoritative state.
- Không nạp scene, shader, model, video, audio, prefab hoặc path từ nguồn không
  tin cậy.

Budget event/byte/time theo frame chỉ là biện pháp performance, không phải hệ
thống chống DoS hoàn chỉnh.

## 10. Checklist phát hành

1. Configure đúng tổ hợp graphics/physics/audio.
2. Build Debug và Release.
3. Chạy CTest.
4. Build package consumer sau khi install.
5. Chạy sample đại diện trên GPU/audio device mục tiêu.
6. Test asset thiếu/hỏng với strict loading.
7. Kiểm tra địa chỉ bind server và firewall.
8. Đóng gói `share/AxisEngine/assets` và DLL/shared library cần thiết.
9. Thêm license trước khi phân phối.

Test hiện mạnh ở headless core, scene, serialization, physics, navigation và
scripting. Coverage tự động còn yếu cho OpenGL thật, ENet session hoàn chỉnh,
FMOD/irrKlang device và WASAPI hardware. Protocol validation và editor file
conflict/project root đã có headless test.

Đọc [audit tiếng Việt](audit/source_audit_2026-07-23.md) hoặc
[audit tiếng Anh](../eng/audit/source_audit_2026-07-23.md) để xem bằng chứng,
severity và khuyến nghị sửa.

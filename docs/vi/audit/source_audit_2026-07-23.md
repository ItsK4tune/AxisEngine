# Audit source AxisEngine — 2026-07-23

> [English](../../eng/audit/source_audit_2026-07-23.md)

> Đây là finding lịch sử tại commit `787f438`. Xem
> [báo cáo khắc phục](remediation_2026-07-23.md) để biết thay đổi đã triển khai.

## Tóm tắt

Phạm vi: commit `787f438`, bao gồm engine/editor, public header, compiler,
sample, test, shader/template, CMake, helper script, CI và tài liệu.

Đã kiểm tra 176 file implementation `.cpp` (khoảng 51.400 dòng), 339 header
(khoảng 18.000 dòng), 52 file sample và 28 file test. Release build của
`axis_engine`, `axis_editor`, `axis_samples` thành công. Test pass 185/185.
SDK đã được install thử; project package-consumer cũng configure và build thành
công qua `find_package(AxisEngine)`.

Không tìm thấy credential nhúng, command injection rõ ràng, marker
`TODO`/`FIXME` cho production implementation, hoặc stub cố tình che giấu.
Các hàm rỗng chủ yếu là lifecycle hook, interface default, constructor/
destructor, Null audio và unsupported capture provider có chủ đích.

Codebase đã khá đầy đủ, nhưng chưa được harden để nhận content không tin cậy
hoặc public network trực tiếp. Ưu tiên cao nhất là network bind/security, giới
hạn và rollback khi load binary scene cũ, và thao tác file an toàn trong editor.

## Kết quả chính

| ID | Mức độ | Vấn đề |
| --- | --- | --- |
| AX-SEC-01 | High | Bind server vào host cụ thể thất bại sẽ tự fallback sang mọi interface |
| AX-SEC-02 | High nếu input không tin cậy | Binary scene cũ có thể cấp phát rất lớn và để lại scene load dở |
| AX-DATA-03 | High | New Asset/Duplicate có thể ghi đè file mà không hỏi conflict |
| AX-SEC-04 | High khi deploy Internet | ENet không có authentication hay encryption |
| AX-PARSE-05 | Medium | Tab indentation trong `.axs` bị parse sai |
| AX-MEM-06 | Medium | Pathfinding không kiểm tra neighbor index |
| AX-MEM-07 | Medium | Lightmap baker tin mesh index/buffer layout |
| AX-TOOL-08 | Medium | `axis_tools.bat` chạy sai path của `axis_compile` trên Visual Studio |
| AX-UX-09 | Medium | File Hierarchy lưu project root nhưng không giới hạn trong root |
| AX-PERF-10 | Low/Medium | Resource Browser scan source directory mỗi frame |
| AX-PATH-11 | Low | `asset://` thiếu file có fallback path gây nhầm lẫn |
| AX-SUPPLY-12 | Low/Medium | GitHub Actions chưa pin bằng commit SHA |
| AX-TEST-13 | Medium | Thiếu integration test cho ENet, OpenGL, editor file, audio hardware |

## Chi tiết và khuyến nghị

### AX-SEC-01 — Network bind mở rộng ngoài ý muốn

`src/network/logic/network_system.cpp:259` retry bằng `ENET_HOST_ANY` nếu bind
host được chỉ định thất bại. Sai địa chỉ hoặc interface tạm mất có thể làm
service lắng nghe trên LAN/WAN ngoài ý muốn.

Nên fail closed. Chỉ fallback khi có option
`allowAnyInterfaceFallback=true`, mặc định false, đồng thời hiển thị rõ địa chỉ
bind cuối cùng trong editor.

### AX-SEC-02 — Binary scene chưa có giới hạn thực tế và transaction

`src/scene/logic/binary_scene_serializer.cpp:28`, `:536`, `:569` cho phép string
khai báo tới 256 MiB, reserve `entityCount` không có giới hạn scene hợp lý, và
legacy loader tạo entity trực tiếp trước khi biết file hợp lệ hoàn toàn.

Nên giới hạn tổng file/payload/entity, so length với số byte còn lại, xử lý
`bad_alloc`, load vào scene tạm rồi chỉ commit sau validation/finalization.

### AX-DATA-03 — Editor có thể làm mất file

`src/editor/panels/file_hierarchy_panel.cpp:88`, `:157` dùng
`overwrite_existing` và `std::ios::trunc`. New Asset hoặc Duplicate trùng tên
sẽ thay file ngay.

Nên sinh tên duy nhất, hỏi xác nhận overwrite với full path, ghi file tạm rồi
atomic rename nếu nền tảng hỗ trợ. Luôn dùng version control khi chạy editor.

### AX-SEC-04 — Network không phải secure protocol

Network hiện cung cấp ENet messaging, stats và transform replication, nhưng
không có identity, authorization, encryption, replay protection hay
authoritative gameplay.

Trước khi public Internet cần handshake/versioning, authenticated session,
message validation, rate limit và secure transport.

### AX-PARSE-05 — Tab làm mất ký tự

`src/core/logic/yaml_parser.cpp:120` dùng cùng biến cho offset trong string và
độ rộng indent. Gặp tab, biến tăng hai rồi substring theo offset mới, làm bỏ
qua ký tự content.

Nên từ chối tab kèm line/column hoặc tách source offset khỏi indent width. Manual
hiện yêu cầu dùng space.

### AX-MEM-06 và AX-MEM-07 — Thiếu bounds check

`src/navigation/logic/pathfinding.cpp:100` truy cập
`navMesh.nodes[neighbor]` mà không kiểm tra range. Validator bắt được trong flow
load scene bình thường, nhưng API public vẫn nhận component tạo từ code.

`src/editor/panels/lighting_panel.cpp:198-221` tính địa chỉ vertex từ index,
stride và buffer mà không validate đầy đủ.

Nên validate ngay tại API tiêu thụ và cung cấp checked vertex accessor dùng
chung cho editor.

### AX-TOOL-08 — Scene compiler helper sai path

`axis_tools.bat:123`, `:142` gọi `build\bin\axis_compile.exe`, trong khi
Visual Studio output là
`build\bin\<Configuration>\axis_compile.exe`.

Nên dùng path có `%COMPILE_BUILD_TYPE%` và giữ flat-path fallback cho generator
single-config.

### AX-UX-09 — Project root chưa được dùng

`m_ProjectRoot` chỉ được gán tại
`src/editor/panels/file_hierarchy_panel.cpp:21`. File Hierarchy vẫn cho đi lên
parent hoặc nhập absolute path rồi tạo, rename, duplicate, delete file.

Nên tách rõ Project mode và Filesystem mode; Project mode phải canonicalize và
khóa bên dưới root.

### AX-PERF-10 — Scan filesystem mỗi frame

Scriptable/State tab trong Resource Browser duyệt directory mỗi frame hiển thị.
Project lớn, network drive hoặc antivirus có thể làm editor giật.

Nên cache kết quả và refresh bằng watcher/event/button; scan lớn chạy background
job rồi handoff kết quả về main thread.

### AX-TEST-13 — Khoảng trống test

185 test hiện cover tốt core, scene, serialization, physics, navigation,
resource, scripting, transform, DDS, video decode và public header. Coverage
thực tế còn thiếu cho:

- ENet server/client và malformed packet;
- OpenGL context/render/shader lifetime;
- editor create/overwrite/delete file;
- FMOD/irrKlang device;
- WASAPI startup/shutdown/hot unplug;
- package consumer trong lệnh test local mặc định.

Nên thêm loopback ENet test, editor test trên temp directory, GPU/audio smoke
lane tùy chọn và fuzz target cho `.axs`, `.axsb`, packet parser.

## Điểm mạnh

- Interface/provider boundary rõ; backend không hỗ trợ fail rõ ràng.
- Initialization rollback và shutdown idempotent.
- Resource cache thread-safe; async decode và main-thread GPU upload; có
  generation check khi cancel.
- Event listener copy-on-write, cho phép subscribe/unsubscribe khi dispatch.
- Job system hỗ trợ nested wait và bắt exception trong task.
- Scene hierarchy chống cycle thông thường, giữ current/previous transform khi
  reparent.
- Scene/batch format có version và test validation.
- Có render-state cache, transient buffer, culling, batching, navigation
  dirty-region rebuild, budget và profiler.
- Public umbrella/package-consumer test; strategy header không được install.

## Thứ tự sửa đề xuất

1. Network bind fail closed và định nghĩa security boundary.
2. Binary scene bounded + transactional; thêm fuzzing.
3. Editor file write có conflict safety và project scope.
4. Bounds check cho pathfinding và lightmap mesh.
5. Sửa tab diagnostic và `axis_tools.bat`.
6. Thêm ENet/editor/GPU/audio integration test.
7. Pin CI action, bật strict warning/static analysis, định kỳ build không
   PCH/unity.
8. Thêm license chính thức.

Chi tiết tiếng Anh đầy đủ tại
[bản audit tiếng Anh](../../eng/audit/source_audit_2026-07-23.md).

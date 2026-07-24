# Khắc phục sau audit AxisEngine — 2026-07-23

> [English](../../eng/audit/remediation_2026-07-23.md)

Tài liệu này ghi lại các thay đổi được triển khai sau báo cáo audit tại commit
`787f438`.

## Đã khắc phục

- Bind network cụ thể hiện fail closed. Chỉ fallback sang mọi interface khi bật
  `allowAnyInterfaceFallback=true`. Editor và networking sample dùng đúng địa
  chỉ bind đã nhập, mặc định loopback thay vì âm thầm chọn mọi interface.
- `NetworkConfig` mặc định là `RequireSecure`. Network không khởi động nếu chưa
  đăng ký và initialize `INetworkSecurityProvider`. ENet thuần phải chọn rõ
  `TrustedNetwork` và sẽ phát cảnh báo.
- Application message và replication dùng protocol envelope có version, giới
  hạn length và sequence check. Security provider phải authenticate peer trước
  connect callback, seal/open packet và authorize từng packet đã decode. Wire
  packet quá lớn bị từ chối trước khi provider parse.
- Binary scene có giới hạn cấu hình được cho file, payload, string và entity.
  Length được so với số byte còn lại trước khi cấp phát. Legacy load rollback
  entity đã tạo và chỉ cập nhật embedded config sau khi finalize thành công.
- `.axs` dùng tab để indent bị từ chối kèm source, line và column.
- Pathfinding bỏ qua neighbor index không hợp lệ.
- Lightmap baker đọc CPU vertex qua API có bounds check.
- `axis_tools.bat` tìm được cả đường dẫn multi-config lẫn flat của
  `axis_compile.exe`.
- Create, duplicate, rename và delete trong editor dùng file service chung,
  giới hạn trong project. Create không ghi đè, duplicate sinh tên duy nhất và
  rename không thay file đã tồn tại.
- File Hierarchy canonicalize navigation/mutation dưới project root, bao gồm
  symlink/reparse point.
- Resource Browser cache danh sách Scriptable/State; chỉ scan khi initialize
  hoặc người dùng refresh.
- Đã dọn lời tường thuật dư thừa/comment dạng agent, phase trang trí,
  commented-out code và comment stale trong các source được review; đồng thời
  thêm quy định comment song ngữ.

## Ranh giới bảo mật còn lại

AxisEngine chưa đi kèm cryptographic provider. Ứng dụng public Internet phải
đăng ký `INetworkSecurityProvider` đã được review, có authentication,
authenticated encryption, replay-safe session state và authorization.
`TrustedNetwork` vẫn không xác thực/mã hóa và chỉ phù hợp với mạng nội bộ được
kiểm soát.

## Kiểm chứng

- Release build: `axis_engine`, `axis_editor`, `axis_samples`, `axis_test`.
- Automated suite: 199/199 test pass.
- Engine, editor và test cũng build thành công khi tắt Unity Build.
- Test mới cover binary scene lỗi/quá lớn, legacy rollback, navmesh neighbor sai,
  tab indentation, editor file conflict/project root và protocol envelope.

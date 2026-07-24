# Quy định về comment

> [English](../../eng/guides/comment_policy.md)

Comment trong AxisEngine chỉ nên giữ thông tin không thể biểu đạt rõ bằng tên,
type, test hoặc control flow.

Giữ comment giải thích:

- lý do thiết kế và phương án đã loại bỏ;
- invariant, ownership, lifetime và thread-safety;
- security hoặc trust boundary;
- binary format, protocol, workaround nền tảng và thuật toán khó thấy;
- public API contract, license và attribution của bên thứ ba.

Không thêm:

- câu tường thuật lặp lại lệnh ngay bên dưới;
- nội dung hội thoại hoặc lịch sử agent đã sửa code như thế nào;
- code bị comment-out hoặc changelog trong implementation;
- banner, số thứ tự phase không đại diện cho boundary kiến trúc thật;
- `TODO`, `FIXME`, `HACK` không có owner hoặc issue được theo dõi.

Mọi cleanup phải được review thủ công. Có thể dùng tìm kiếm để lập danh sách
ứng viên, nhưng không được tự động xóa comment vì công cụ không phân biệt được
invariant quan trọng với lời tường thuật dư thừa.

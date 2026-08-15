# Hướng dẫn Lập trình Scriptable API & Vòng đời Hành vi Entity

> [English](../../eng/scripting/scriptable_api.md) | [Quản lý State API](../state/state_api.md) | [Tra cứu Component Reference](../guides/components_reference.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine hỗ trợ lập trình hành vi entity bằng C++ bằng cách kế thừa lớp `Scriptable`. Được gắn vào entity qua `ScriptComponent`, các instance `Scriptable` nhận các callback vòng đời, hook va chạm vật lý, sự kiện input và bộ đếm thời gian thực thi hoãn lại (`Invoke`).

### Các Giai đoạn trong Vòng đời `Scriptable`
1. **Ràng buộc (`Initialize`)**: Engine liên kết handle `entt::entity` và con trỏ `Scene*` đang hoạt động với instance script.
2. **Khởi tạo (`OnCreate`)**: Thực thi 1 lần khi entity và script vào scene đang hoạt động. Thích hợp cho việc truy vấn component và thiết lập trạng thái ban đầu.
3. **Kích hoạt & Tắt (`OnEnable` / `OnDisable`)**: Thực thi bất cứ khi nào `SetEnabled(true)` hoặc `SetEnabled(false)` được gọi. Điều khiển việc thực thi cập nhật.
4. **Thực thi Khung hình (`OnUpdate`, `OnFixedUpdate`)**:
   - `OnUpdate(float dt)`: Thực thi trong các frame tick logic biến thiên cho di chuyển, bộ đếm thời gian và xử lý input.
   - `OnFixedUpdate(float fixedDt)`: Thực thi trong các bước thời gian vật lý cố định cho tác dụng lực và cập nhật kinematic.
5. **Tương tác Va chạm Vật lý (`OnCollision*`, `OnTrigger*`)**:
   - Va chạm body rắn: `OnCollisionEnter(other)`, `OnCollisionStay(other)`, `OnCollisionExit(other)`.
   - Vùng kích hoạt Trigger: `OnTriggerEnter(other)`, `OnTriggerStay(other)`, `OnTriggerExit(other)`.
6. **Sự kiện Input (`OnKey*`, `OnMouse*`)**:
   - Bàn phím: `OnKeyPress(key)`, `OnKeyRelease(key)`.
   - Chuột: `OnMouseButtonPress(btn)`, `OnMouseButtonRelease(btn)`, `OnMouseEnter()`, `OnMouseExit()`, `OnMouseOver()`, `OnMouseClicked(btn)`.
7. **Hủy bỏ (`OnDestroy`)**: Thực thi khi component script hoặc entity bị xóa bỏ.

---

## 2. Cách dùng

1. **Kế thừa `Scriptable`**: Tạo lớp kế thừa `Scriptable` và ghi đè các callback vòng đời cần thiết (`OnCreate()`, `OnUpdate(float dt)`, `OnCollisionEnter(other)`, `OnDestroy()`).
2. **Đăng ký Script Factory**: Gọi `app.RegisterScript<MyScript>("MyScript")` bên trong `Application::RegisterUserScripts()`.
3. **Gắn vào Entity**: Gọi `entity.AddScript<MyScript>("MyScript")` trong mã C++ hoặc chỉ định `Script:` trong file scene `.axs`.
4. **Lên lịch Thực thi Hoãn lại**: Gọi `Invoke([this]() { DoSomething(); }, delaySeconds)` để lên lịch thực thi callback bất đồng bộ.

---

## 3. Ví dụ

### Ví dụ Script Player Controller với Đầy đủ Callback Vòng đời
```cpp
#include <axis_sdk.h>

class PlayerScript final : public Scriptable {
private:
    float m_moveSpeed = 6.0f;
    int m_health = 100;

public:
    void OnCreate() override {
        AXIS_LOG_INFO("PlayerScript duoc khoi tao tren entity ID: " + std::to_string(static_cast<uint32_t>(m_Entity)));

        // Lên lịch hồi máu hoãn lại sau 5 giây
        Invoke([this]() {
            m_health = 100;
            AXIS_LOG_INFO("Player da duoc hoan blood!");
        }, 5.0f);
    }

    void OnEnable() override {
        AXIS_LOG_INFO("PlayerScript duoc kich hoat");
    }

    void OnUpdate(float dt) override {
        auto& input = InputManager::Get();
        auto& transform = GetComponent<TransformComponent>();

        if (input.IsKeyDown(KeyCode::W)) {
            transform.Translate(Vector3(0.0f, 0.0f, m_moveSpeed * dt));
        }
    }

    void OnFixedUpdate(float fixedDt) override {
        // Cập nhật bước vật lý
    }

    void OnTriggerEnter(entt::entity other) override {
        if (CompareTag(other, "Pickup")) {
            AXIS_LOG_INFO("Da thu thap vat pham!");
            Destroy(other);
        }
    }

    void OnDisable() override {
        AXIS_LOG_INFO("PlayerScript bi tat");
    }

    void OnDestroy() override {
        AXIS_LOG_INFO("PlayerScript bi huy");
    }
};

void RegisterAppScripts(Application& app) {
    app.RegisterScript<PlayerScript>("PlayerScript");
}
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Callback Vòng đời `Scriptable`

| Tên Phương thức | Thời điểm Gọi / Điều kiện Kích hoạt | Mô tả |
| :--- | :--- | :--- |
| `Initialize(entity, scene)` | Ràng buộc Hệ thống | Liên kết handle entity EnTT và con trỏ scene |
| `OnCreate()` | Khởi tạo Entity | Thực thi 1 lần khi script entity vào scene đang hoạt động |
| `OnEnable()` | Kích hoạt | Thực thi khi script được bật (`SetEnabled(true)`) |
| `OnDisable()` | Tắt Kích hoạt | Thực thi khi script bị tắt (`SetEnabled(false)`) |
| `OnUpdate(float dt)` | Mỗi Khung hình | Thực thi trong vòng lặp cập nhật logic biến thiên |
| `OnFixedUpdate(float fixedDt)` | Bước Vật lý Cố định | Thực thi trong bước vật lý cố định Bullet 3D |
| `OnCollisionEnter(other)` | Va chạm Body Rắn | Kích hoạt khi bắt đầu xảy ra va chạm vật lý |
| `OnCollisionStay(other)` | Va chạm Body Rắn | Kích hoạt liên tục trong khi va chạm còn duy trì |
| `OnCollisionExit(other)` | Va chạm Body Rắn | Kích hoạt khi va chạm vật lý kết thúc |
| `OnTriggerEnter(other)` | Vùng Kích hoạt Trigger | Kích hoạt khi entity đi vào vùng trigger |
| `OnTriggerStay(other)` | Vùng Kích hoạt Trigger | Kích hoạt khi entity ở bên trong vùng trigger |
| `OnTriggerExit(other)` | Vùng Kích hoạt Trigger | Kích hoạt khi entity đi ra khỏi vùng trigger |
| `OnKeyPress(key)` | Input Bàn phím | Kích hoạt khi một phím bàn phím được nhấn xuống |
| `OnKeyRelease(key)` | Input Bàn phím | Kích hoạt khi một phím bàn phím được thả ra |
| `OnMouseEnter()` | Rê Chuột | Kích hoạt khi con trỏ chuột đi vào phạm vi UI/entity |
| `OnMouseExit()` | Rê Chuột | Kích hoạt khi con trỏ chuột rời khỏi phạm vi UI/entity |
| `OnMouseClicked(btn)` | Click Chuột | Kích hoạt khi click chuột vào phạm vi UI/entity |
| `OnDestroy()` | Hủy bỏ | Thực thi khi script hoặc entity bị gỡ bỏ |

### Bảng Tra cứu Các Hàm Hỗ trợ trong `Scriptable`

| Tên Phương thức | Kiểu Trả về | Mô tả |
| :--- | :--- | :--- |
| `GetComponent<T>()` | `T&` | Lấy tham chiếu tới component `T` trên entity hiện tại |
| `HasComponent<T>()` | `bool` | Kiểm tra xem entity hiện tại có component `T` hay không |
| `GetScript<T>(targetEntity)` | `T*` | Lấy con trỏ tới script `T` trên entity mục tiêu |
| `SetEnabled(enabled)` | `void` | Bật hoặc tắt thực thi tick script (`OnEnable`/`OnDisable`) |
| `SetRunWhenPaused(run)` | `void` | Cấu hình cho phép script tiếp tục tick khi game bị tạm dừng |
| `Invoke(callback, delaySeconds)` | `void` | Lên lịch thực thi hoãn lại một lambda callback |
| `Spawn(name, pos, rot, scale)` | `entt::entity` | Sinh ra một entity mới vào scene đang hoạt động |
| `Destroy(targetEntity)` | `void` | Gỡ bỏ entity mục tiêu khỏi scene đang hoạt động |
| `CompareTag(entity, tag)` | `bool` | Kiểm tra xem entity mục tiêu có khớp với chuỗi tag hay không |
| `CompareName(entity, name)` | `bool` | Kiểm tra xem entity mục tiêu có khớp với chuỗi tên hay không |

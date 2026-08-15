# Hướng dẫn Quản lý State Machine API & Vòng đời State

> [English](../../eng/state/state_api.md) | [Lập trình Scriptable API](../scripting/scriptable_api.md) | [Bắt đầu Nhanh](../core/getting_started.md) | [Mục lục Tài liệu](../INDEX.md)

---

## 1. Giới thiệu

AxisEngine quản lý luồng ứng dụng cấp cao (Boot, Main Menu, Gameplay, Pause Menu, Game Over) bằng cách sử dụng **State Machine dạng Stack** (`StateMachine`). Mỗi state ứng dụng được viết bằng cách kế thừa lớp `State` và triển khai các phương thức callback vòng đời của nó.

### Các Giai đoạn trong Vòng đời State
1. **Khởi tạo & Đẩy State (`OnEnter`)**: Thực thi khi instance state được đẩy vào stack. Cấp phát tài nguyên riêng cho state và nạp scene.
2. **Thực thi (`OnUpdate`, `OnFixedUpdate`, `OnRender`)**:
   - `OnUpdate(float dt)`: Được gọi mỗi frame tick biến thiên để xử lý logic game, bộ đếm thời gian và nhận input.
   - `OnFixedUpdate(float fixedDt)`: Được gọi trong các bước vật lý cố định cho các truy vấn vật lý chính xác.
   - `OnRender()`: Được gọi trong phase dựng hình đồ họa.
   - `OnRenderDebug()`: Được gọi trong phase dựng hình lớp phủ debug.
3. **Tạm dừng (`OnPause`)**: Thực thi khi một state mới được đẩy đè lên trên state hiện tại (ví dụ: đẩy Menu Pause dạng modal lên trên Gameplay). State bị tạm dừng vẫn được giữ lại trong bộ nhớ.
4. **Tiếp tục (`OnResume`)**: Thực thi khi state modal đè phía trên bị gỡ bỏ, khôi phục state bên dưới trở lại làm state hoạt động chính trên cùng.
5. **Hủy bỏ & Thoát (`OnExit`)**: Thực thi khi state bị gỡ khỏi stack hoặc bị giải phóng qua `ChangeState()`. Giải phóng tài nguyên state và hủy scene.

---

## 2. Cách dùng

1. **Kế thừa `State`**: Kế thừa từ `State` và ghi đè `OnEnter()`, `OnUpdate(float dt)`, `OnFixedUpdate(float fixedDt)`, `OnRender()`, `OnPause()`, `OnResume()`, và `OnExit()`.
2. **Đẩy State Modal Lớp phủ**: Gọi `Application::Get().PushState<PauseState>()` để đẩy menu tạm dừng đè lên gameplay. State hiện tại nhận `OnPause()`, và `PauseState` nhận `OnEnter()`.
3. **Gỡ State Lớp phủ**: Gọi `Application::Get().PopState()` để quay lại state bên dưới. `PauseState` nhận `OnExit()`, và state bên dưới nhận `OnResume()`.
4. **Chuyển đổi Chế độ Chính**: Gọi `Application::Get().ChangeState<GameplayState>()` để xóa sạch toàn bộ state trong stack và chuyển sang `GameplayState`.

---

## 3. Ví dụ

### Ví dụ Chuyển đổi Stack State Machine Hoàn chỉnh
```cpp
#include <axis_sdk.h>

class PauseState final : public State {
public:
    std::string GetName() const override { return "PauseState"; }

    void OnEnter() override {
        AXIS_LOG_INFO("PauseState::OnEnter - Menu pause modal da mo");
    }

    void OnUpdate(float dt) override {
        if (InputManager::Get().IsKeyPressed(KeyCode::Escape)) {
            // Gỡ menu pause modal và tiếp tục GameplayState bên dưới
            Application::Get().PopState();
        }
    }

    void OnRender() override {}

    void OnExit() override {
        AXIS_LOG_INFO("PauseState::OnExit - Menu pause modal da dong");
    }
};

class GameplayState final : public State {
public:
    std::string GetName() const override { return "GameplayState"; }

    void OnEnter() override {
        AXIS_LOG_INFO("GameplayState::OnEnter - Dang nap scene man choi");
        SceneManager::Get().LoadScene("scenes/level1.axs");
    }

    void OnUpdate(float dt) override {
        if (InputManager::Get().IsKeyPressed(KeyCode::Escape)) {
            // Đẩy PauseState lên trên mà không hủy GameplayState
            Application::Get().PushState<PauseState>();
        }
    }

    void OnFixedUpdate(float fixedDt) override {
        // Cập nhật bước vật lý cố định
    }

    void OnRender() override {}

    void OnPause() override {
        AXIS_LOG_INFO("GameplayState::OnPause - Bi tam dung boi menu pause modal");
    }

    void OnResume() override {
        AXIS_LOG_INFO("GameplayState::OnResume - Tiếp tục gameplay sau khi dong menu pause");
    }

    void OnExit() override {
        AXIS_LOG_INFO("GameplayState::OnExit - Huy scene man choi");
    }
};
```

---

## 4. Tra cứu Param, Setting & API Reference

### Bảng Tra cứu Các Callback Vòng đời State

| Tên Phương thức | Thời điểm Gọi | Mục đích |
| :--- | :--- | :--- |
| `OnEnter()` | Kích hoạt State | Được gọi 1 lần khi state được đẩy vào stack hoạt động |
| `OnUpdate(float dt)` | Vòng lặp Khung hình Biến thiên | Được gọi mỗi frame tick cho state trên cùng |
| `OnFixedUpdate(float fixedDt)` | Bước Vật lý Cố định | Được gọi mỗi fixed timestep vật lý |
| `OnRender()` | Phase Dựng hình Khung hình | Được gọi trong phase dựng hình đồ họa |
| `OnRenderDebug()` | Phase Dựng hình Debug | Được gọi trong phase dựng hình lớp phủ debug |
| `OnPause()` | Tạm dừng Modal | Được gọi khi có một state mới được đẩy đè lên trên |
| `OnResume()` | Khôi phục Modal | Được gọi khi state đè phía trên bị gỡ và state này trở lại đỉnh stack |
| `OnExit()` | Hủy bỏ State | Được gọi khi state bị gỡ khỏi stack hoặc thay thế |

### Bảng Tra cứu Thao tác Stack StateMachine

| Tên Phương thức | Hành vi Stack Mục tiêu | Trình tự Callback Vòng đời Được Kích hoạt |
| :--- | :--- | :--- |
| `PushState<T>()` | Đẩy state mới `T` lên đỉnh stack | State hiện tại nhận `OnPause()`; state mới nhận `OnEnter()` |
| `PopState()` | Gỡ bỏ state trên cùng khỏi stack | State trên cùng nhận `OnExit()`; state bên dưới nhận `OnResume()` |
| `ChangeState<T>()` | Thay thế toàn bộ stack bằng state `T` | Tất cả state hoạt động nhận `OnExit()`; state mới nhận `OnEnter()` |
| `Clear()` | Hủy toàn bộ state trên stack | Gọi `OnExit()` trên tất cả state và giải phóng bộ nhớ |
| `GetCurrentState()` | Truy vấn state đang hoạt động | Trả về con trỏ con `State*` tới state trên cùng |
| `GetStates()` | Truy vấn toàn bộ danh sách state | Trả về `std::vector<State*>` sắp xếp từ dưới lên đỉnh |

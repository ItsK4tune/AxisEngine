# Quản lý thiết bị

> [English](../../eng/guides/device_management.md)

Monitor và input dùng `IDeviceManager`. Playback và microphone có interface
riêng vì lifecycle/capability khác nhau.

## 1. IDeviceManager

```cpp
struct DeviceInfo {
    std::string id;
    std::string name;
    DeviceType type;
    bool isDefault;
};

virtual std::vector<DeviceInfo> GetAllDevices() const = 0;
virtual DeviceInfo GetCurrentDevice() const = 0;
virtual bool SetActiveDevice(const std::string& deviceId) = 0;
```

Luôn lưu/chọn bằng `id` ổn định, không dùng vị trí trong vector enumerate.

## 2. Manager được hỗ trợ

### MonitorManager

Quản lý GLFW window/monitor, resize, fullscreen/windowed và vsync.

### InputManager

- Keyboard, mouse, mapped gamepad và raw joystick.
- ID `gamepad_N`, `joystick_N`; thiết bị active mất kết nối sẽ fallback merged input.
- Axis hỗ trợ `leftx`, `lefty`, `rightx`, `righty`, `lefttrigger`,
  `righttrigger`; đọc bằng `EngineAccessor::GetAxis`.
- `GAMEPAD_DEAD_ZONE` áp radial filtering.
- Button hỗ trợ held/pressed/released. Rumble chưa có API công khai.

### Audio

- `IAudioEngine`: playback/output.
- `IAudioCaptureService`: enumerate microphone, calibration, level và pulse.
- Xem [thu âm microphone](audio_capture.md).

## 3. Debug

Xem [debug và phím editor](debug_system.md).

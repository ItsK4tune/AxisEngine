# Device Management

Axis uses `IDeviceManager` for monitors and input devices. Audio playback and microphone capture have
separate interfaces because their lifecycles and capabilities are different.

## 1. IDeviceManager Interface

All device managers implement `IDeviceManager`, providing a standard way to enumerate and select devices.

```cpp
struct DeviceInfo {
    std::string id;       // Unique ID used for selection
    std::string name;     // Human-readable name
    DeviceType type;      // Monitor, Keyboard, Mouse, Joystick, or Gamepad
    bool isDefault;       // Is this the system default?
};

// Interface
virtual std::vector<DeviceInfo> GetAllDevices() const = 0;
virtual DeviceInfo GetCurrentDevice() const = 0;
virtual bool SetActiveDevice(const std::string& deviceId) = 0;
```

## 2. Supported Managers

### MonitorManager
- Manages the GLFW Window and Monitor selection.
- Supports resizing, fullscreen/windowed modes, vsync.
- **Config**: Set via the `Config` block in your `.axs` scene file.

### InputManager
- Manages keyboard, mouse, mapped gamepads, and raw joysticks.
- Enumerates gamepads as `gamepad_N` and raw joysticks as `joystick_N`; disconnected active devices fall back to merged input.
- Supports mapped buttons and axes. Bind axes with `GamepadAxis: leftx`, `lefty`, `rightx`, `righty`, `lefttrigger`, or `righttrigger`, then read the normalized value with `EngineAccessor::GetAxis`. `GAMEPAD_DEAD_ZONE` controls radial magnitude filtering for each axis.
- `GamepadButton` bindings support held, pressed, and released action queries. Axis/dead-zone and rumble APIs are not currently exposed.

### Audio services
- `IAudioEngine` owns playback and reports backend-specific output-device capabilities.
- `IAudioCaptureService` owns microphone enumeration, selection, calibration, and level/pulse snapshots.
- See [Microphone capture](audio_capture.md).

## 3. Debugging

For information on debugging devices and using the Debug System (F1-F12 keys), please refer to default **[Debug System Guide](debug_system.md)**.


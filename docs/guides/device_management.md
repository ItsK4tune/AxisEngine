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
- Manages Keyboards, Mice, and Joysticks.
- Currently supports listing devices.

### Audio services
- `IAudioEngine` owns playback and reports backend-specific output-device capabilities.
- `IAudioCaptureService` owns microphone enumeration, selection, calibration, and level/pulse snapshots.
- See [Microphone capture](audio_capture.md).

## 3. Debugging

For information on debugging devices and using the Debug System (F1-F12 keys), please refer to default **[Debug System Guide](debug_system.md)**.


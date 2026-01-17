# Device Management

The AXIS Engine uses a unified `IDeviceManager` interface to handle hardware devices (Monitors, Audio, Inputs).

## 1. IDeviceManager Interface

All device managers implement `IDeviceManager`, providing a standard way to enumerate and select devices.

```cpp
struct DeviceInfo {
    std::string id;       // Unique ID used for selection
    std::string name;     // Human-readable name
    DeviceType type;      // Monitor, Keyboard, Mouse, AudioOutput, etc.
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
- **Config**: Set via `configuration/settings.json` (`width`, `height`, `monitorIndex`).

### SoundManager
- Manages Audio Output devices via `irrKlang`.
- **Config**: Set via `configuration/settings.json` (`audioDevice`).

### InputManager
- Manages Keyboards, Mice, and Joysticks.
- Currently supports listing devices.

## 3. Debugging

For information on debugging devices and using the Debug System (F1-F12 keys), please refer to default **[Debug System Guide](debug_system.md)**.


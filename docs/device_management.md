# Device Management
![AXIS Engine Logo](../assets/logo.png)

**Engine**: AXIS Engine  
**Contributor**: Duong "Caftun" Nguyen

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

## 3. Debugging Features

The engine includes built-in shortcuts for debugging hardware and physics.

### Device List (F2)
Press **F2** while the application is running to print a list of all detected devices to the console.

**Output Example:**
```text
========== DEVICE LIST ==========
Monitors:
  [0] Generic PnP Monitor (Default)
  [1] DELL U2414H
Inputs:
  [keyboard_0] Primary Keyboard (Default)
  [mouse_0] Primary Mouse (Default)
Audio:
  [default] Default DirectSound Device (Default)
  [AD12345] Headphones (Realtek Audio)
=================================
```
You can use the ID shown (e.g., `AD12345`) in `settings.json` to select that specific device.

### Physics Debug (F1)
Press **F1** to toggle the Physics Debug Drawer.
- **Enabled**: Wireframes of physics colliders (green lines) are drawn over the scene.
- **Disabled**: Standard rendering.

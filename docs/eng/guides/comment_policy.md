# Code Comment & Documentation Standards Guide

> [Tiếng Việt](../../vi/guides/comment_policy.md) | [Public API Surface](../core/api_surface.md) | [Documentation Index](../INDEX.md)

---

## 1. Introduction

AxisEngine maintains clean, consistent C++ code comments across all public headers and internal source files using Doxygen formatting tags.

---

## 2. How to Use

1. **Header Briefs**: Use `/// @brief` above class, struct, or function declarations.
2. **Parameters**: Document parameters with `/// @param name Description`.
3. **Return Values**: Document returns using `/// @return Description`.

---

## 3. Examples

### Doxygen Comment Example
```cpp
#include <axis_sdk.h>

/// @brief Represents a player behavior script in the game.
class PlayerScript final : public Scriptable {
public:
    /// @brief Ticks script logic frame update.
    /// @param dt Frame delta time in seconds.
    void OnUpdate(float dt) override {
        // Implementation
    }
};
```

---

## 4. API & Configuration Reference

### Doxygen Formatting Tags Reference

| Tag Name | Purpose | Example |
| :--- | :--- | :--- |
| `/// @brief` | Short summary of symbol | `/// @brief Computes spatial culling.` |
| `/// @param` | Parameter documentation | `/// @param dt Frame delta time in seconds.` |
| `/// @return` | Return value description | `/// @return True if initialized successfully.` |

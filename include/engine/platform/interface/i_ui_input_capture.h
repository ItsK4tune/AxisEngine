#pragma once

// Optional UI/editor bridge used by gameplay input without depending on a
// concrete immediate-mode UI backend.
class IUIInputCapture
{
public:
    virtual ~IUIInputCapture() = default;
    virtual bool WantsPointerInput() const = 0;
    virtual bool WantsTextInput() const = 0;
};

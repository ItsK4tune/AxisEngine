#pragma once

enum class ApplicationLifecycle
{
    Created,
    Initializing,
    Initialized,
    Running,
    ShuttingDown,
    Stopped,
    Failed
};

class IApplicationLifecycle
{
public:
    virtual ~IApplicationLifecycle() = default;
    virtual ApplicationLifecycle GetLifecycle() const = 0;
};

inline const char* ApplicationLifecycleName(ApplicationLifecycle lifecycle)
{
    switch (lifecycle)
    {
        case ApplicationLifecycle::Created:
            return "Created";
        case ApplicationLifecycle::Initializing:
            return "Initializing";
        case ApplicationLifecycle::Initialized:
            return "Initialized";
        case ApplicationLifecycle::Running:
            return "Running";
        case ApplicationLifecycle::ShuttingDown:
            return "ShuttingDown";
        case ApplicationLifecycle::Stopped:
            return "Stopped";
        case ApplicationLifecycle::Failed:
            return "Failed";
    }
    return "Unknown";
}

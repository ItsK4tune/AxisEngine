#pragma once

#include <platform/interface/key.h>

// Optional pre-gameplay keyboard router. It is queried directly from the
// platform callback so editor commands can be consumed before raw events,
// InputManager actions, and scripts observe them.
class IKeyboardInputRouter
{
public:
    virtual ~IKeyboardInputRouter() = default;
    virtual bool ShouldConsumeKey(Key key) const = 0;
};

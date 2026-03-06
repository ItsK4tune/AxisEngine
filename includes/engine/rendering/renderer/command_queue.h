#pragma once

#include <vector>
#include <functional>

using RenderCommand = std::function<void()>;

class CommandQueue {
public:
    void Submit(RenderCommand cmd);
    void Execute();
    void Clear();
    bool IsEmpty() const { return m_Commands.empty(); }

private:
    std::vector<RenderCommand> m_Commands;
};

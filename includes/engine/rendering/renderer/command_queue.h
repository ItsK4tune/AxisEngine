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

    void Merge(CommandQueue& other)
    {
        m_Commands.insert(m_Commands.end(), 
            std::make_move_iterator(other.m_Commands.begin()), 
            std::make_move_iterator(other.m_Commands.end()));
        other.Clear();
    }
    std::vector<RenderCommand> m_Commands;
};

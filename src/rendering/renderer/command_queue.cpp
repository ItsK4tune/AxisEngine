#include <rendering/renderer/command_queue.h>

void CommandQueue::Submit(RenderCommand cmd)
{
    m_Commands.push_back(std::move(cmd));
}

void CommandQueue::Execute()
{
    for (auto& cmd : m_Commands)
    {
        cmd();
    }
}

void CommandQueue::Clear()
{
    m_Commands.clear();
}

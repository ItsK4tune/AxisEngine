#pragma once

#include <scene/logic/scene.h>
#include <entt/entt.hpp>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

class ECSCommandBuffer
{
public:
    using Command = std::function<void(Scene&)>;

    void Add(Command command)
    {
        m_Commands.push_back(std::move(command));
    }

    template <typename T, typename Fn>
    void Patch(entt::entity entity, Fn&& patch)
    {
        m_Commands.emplace_back([entity, patch = std::forward<Fn>(patch)](Scene& scene) mutable {
            if (!scene.IsValid(entity))
                return;

            if (auto* component = scene.TryGetComponent<T>(entity))
            {
                patch(*component);
            }
        });
    }

    void Apply(Scene& scene)
    {
        for (auto& command : m_Commands)
        {
            command(scene);
        }
        m_Commands.clear();
    }

    void Clear()
    {
        m_Commands.clear();
    }

    bool Empty() const
    {
        return m_Commands.empty();
    }

    size_t Size() const
    {
        return m_Commands.size();
    }

private:
    std::vector<Command> m_Commands;
};

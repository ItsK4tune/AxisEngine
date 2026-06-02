#pragma once
#include <glm/glm.hpp>
#include <algorithm>
#include <cstdint>
#include <vector>

class Shader;

static constexpr int MAX_DRAW_CMD_UNIFORMS = 8;

struct RenderDrawCommand
{
    int layer = 0;
    uint32_t shaderId = 0;
    uint32_t vao = 0;
    uint32_t ebo = 0;
    uint32_t count = 0;

    Shader* shader = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    uint32_t texture0 = 0;
    glm::vec4 tintColor = glm::vec4(1.0f);

    struct UniformSlot
    {
        int location = -1;
        union
        {
            uint32_t uintVal;
            float floatVal;
        };
        enum Type : uint8_t
        {
            UNUSED = 0,
            UINT,
            FLOAT
        } type = UNUSED;
    };
    UniformSlot uniforms[MAX_DRAW_CMD_UNIFORMS];
    int uniformCount = 0;

    void SetUniformUInt(int location, uint32_t value)
    {
        if (uniformCount < MAX_DRAW_CMD_UNIFORMS && location != -1)
        {
            auto& slot = uniforms[uniformCount++];
            slot.location = location;
            slot.uintVal = value;
            slot.type = UniformSlot::UINT;
        }
    }

    void SetUniformFloat(int location, float value)
    {
        if (uniformCount < MAX_DRAW_CMD_UNIFORMS && location != -1)
        {
            auto& slot = uniforms[uniformCount++];
            slot.location = location;
            slot.floatVal = value;
            slot.type = UniformSlot::FLOAT;
        }
    }

    bool operator<(const RenderDrawCommand& other) const
    {
        if (layer != other.layer)
            return layer < other.layer;
        if (shaderId != other.shaderId)
            return shaderId < other.shaderId;
        return vao < other.vao;
    }
};

class RenderCommandBuffer
{
public:
    void Submit(const RenderDrawCommand& cmd)
    {
        m_Commands.push_back(cmd);
    }

    void Sort()
    {
        std::sort(m_Commands.begin(), m_Commands.end());
    }

    void Clear()
    {
        m_Commands.clear();
    }

    const std::vector<RenderDrawCommand>& GetCommands() const
    {
        return m_Commands;
    }

private:
    std::vector<RenderDrawCommand> m_Commands;
};

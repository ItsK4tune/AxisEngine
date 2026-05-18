#pragma once
#include <render/interface/i_graphics_context.h>
#include <render/logic/material_renderer.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>

// Fixed-slot uniform storage — eliminates per-draw heap allocation
// Slots are pre-assigned by convention. Add new slots at the end.
static constexpr int MAX_DRAW_CMD_UNIFORMS = 8;

struct RenderDrawCommand
{
    int layer = 0;          // For UI/Transparent sorting (lower is first)
    uint32_t shaderId = 0;  // For state sorting
    uint32_t vao = 0;       // For state sorting
    uint32_t ebo = 0;       // Element buffer
    uint32_t count = 0;     // Indices count

    Shader* shader = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);

    // Optional: Textures or Material data
    uint32_t texture0 = 0;
    glm::vec4 tintColor = glm::vec4(1.0f);

    // Fixed-slot uniforms — zero allocation, O(1) access
    struct UniformSlot
    {
        int location = -1;  // pre-resolved via Shader::GetUniformLocation
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

    // Commands can be sorted: Layer -> Shader -> VAO
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

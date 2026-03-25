#pragma once
#include <glm/glm.hpp>
#include <resource/unit/shader.h>
#include <render/logic/material_renderer.h>
#include <render/interface/i_graphics_context.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <map>
#include <string>

struct RenderDrawCommand {
    int layer = 0;           // For UI/Transparent sorting (lower is first)
    uint32_t shaderId = 0;   // For state sorting
    uint32_t vao = 0;        // For state sorting
    uint32_t ebo = 0;        // Element buffer
    uint32_t count = 0;      // Indices count
    
    Shader* shader = nullptr;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
    
    // Optional: Textures or Material data
    uint32_t texture0 = 0;
    glm::vec4 tintColor = glm::vec4(1.0f);
    
    std::map<std::string, uint32_t> uintUniforms;
    std::map<std::string, float> floatUniforms;
    
    // Commands can be sorted: Layer -> Shader -> VAO
    bool operator<(const RenderDrawCommand& other) const {
        if (layer != other.layer) return layer < other.layer;
        if (shaderId != other.shaderId) return shaderId < other.shaderId;
        return vao < other.vao;
    }
};

class RenderCommandBuffer {
public:
    void Submit(const RenderDrawCommand& cmd) {
        m_Commands.push_back(cmd);
    }
    
    void Sort() {
        std::sort(m_Commands.begin(), m_Commands.end());
    }
    
    void Clear() {
        m_Commands.clear();
    }
    
    const std::vector<RenderDrawCommand>& GetCommands() const {
        return m_Commands;
    }

private:
    std::vector<RenderDrawCommand> m_Commands;
};

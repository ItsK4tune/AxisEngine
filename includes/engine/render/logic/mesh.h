#pragma once

#include <core/unit/aabb.h>
#include <glm/glm.hpp>
#include <render/logic/shader.h>
#include <render/type/graphics_types.h>
#include <string>
#include <vector>

class IBufferManager;
class IDrawContext;
class ITextureManager;

#define GLM_ENABLE_EXPERIMENTAL


#define MAX_BONE_INFLUENCE 4


class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;
    AABB aabb;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, bool setupGPU = true);
    ~Mesh();
    void setupMesh();
    void Draw(Shader &shader, bool bindTextures = true);
    void DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models, bool bindTextures = true);

    bool IsInitialized() const { return m_Initialized; }

    static void SetManagers(IBufferManager* buf, ITextureManager* tex, IDrawContext* draw);
    static IBufferManager& GetBufferManager() { return *s_BufferManager; }
    static ITextureManager& GetTextureManager() { return *s_TextureManager; }
    static IDrawContext& GetDrawContext() { return *s_DrawContext; }

private:
    unsigned int VBO, EBO, instanceVBO;
    bool m_Initialized = false;

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;
};
#pragma once

#include <core/unit/aabb.h>
#include <render/type/graphics_types.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
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
    std::vector<uint8_t> m_VertexData;
    size_t m_VertexCount;
    size_t m_VertexStride;
    bool m_IsSkinned;

    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;
    AABB aabb;

    Mesh(std::vector<uint8_t> vertexData, size_t vertexCount, size_t vertexStride, bool isSkinned,
         std::vector<unsigned int> indices, std::vector<Texture> textures, bool setupGPU = true);
    ~Mesh();
    void setupMesh();
    void Draw(Shader& shader, bool bindTextures = true);
    void DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models, bool bindTextures = true);

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    static void SetManagers(IBufferManager* buf, ITextureManager* tex, IDrawContext* draw);
    static IBufferManager& GetBufferManager()
    {
        return *s_BufferManager;
    }
    static ITextureManager& GetTextureManager()
    {
        return *s_TextureManager;
    }
    static IDrawContext& GetDrawContext()
    {
        return *s_DrawContext;
    }

private:
    unsigned int VBO, EBO, instanceVBO;
    bool m_Initialized = false;
    size_t m_InstanceBufferCapacity = 0;

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;
};

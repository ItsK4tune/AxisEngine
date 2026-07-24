#pragma once

#include <core/unit/aabb.h>
#include <render/type/graphics_types.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class IBufferManager;
class IDrawContext;
class ITextureManager;
class TransientBufferRing;


#define MAX_BONE_INFLUENCE 4

struct MeshInstanceData
{
    glm::mat4 model{1.0f};
    uint32_t entityId = 0;
};

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
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    void setupMesh();
    void Draw(Shader& shader, bool bindTextures = true);
    void DrawInstanced(Shader& shader, const std::vector<MeshInstanceData>& instances, bool bindTextures = true);

    bool IsInitialized() const
    {
        return m_Initialized;
    }

    bool HasCpuVertexData() const
    {
        return !m_VertexData.empty();
    }
    bool CopyCpuVertex(size_t vertexIndex, void* destination, size_t byteCount) const;
    glm::vec3 GetPosition(size_t vertexIndex) const;
    void ReleaseCpuVertexData();

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
    std::unique_ptr<TransientBufferRing> m_InstanceUpload;
    std::vector<glm::vec3> m_CompactPositions;

    static IBufferManager* s_BufferManager;
    static ITextureManager* s_TextureManager;
    static IDrawContext* s_DrawContext;

    void ReleaseGpuResources() noexcept;
};

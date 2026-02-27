#pragma once

#include <glm/glm.hpp>
#include <graphic/core/shader.h>
#include <math/aabb.h>
#include <string>
#include <vector>

class IBufferManager;
class ITextureManager;
class IDrawContext;

#define MAX_BONE_INFLUENCE 4

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture
{
    unsigned int id = 0;
    std::string type;
    std::string path;

    unsigned char* pixelData = nullptr;
    int width = 0, height = 0, nrComponents = 0;
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO;
    AABB aabb;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures, bool setupGPU = true);
    void setupMesh();
    void Draw(Shader &shader);
    void DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models);

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

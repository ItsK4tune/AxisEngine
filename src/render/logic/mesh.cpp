#include <algorithm>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <resource/unit/mesh.h>
#include <render/type/graphics_types.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <core/logic/logger.h>

IBufferManager* Mesh::s_BufferManager = nullptr;
ITextureManager* Mesh::s_TextureManager = nullptr;
IDrawContext* Mesh::s_DrawContext = nullptr;

void Mesh::SetManagers(IBufferManager* buf, ITextureManager* tex, IDrawContext* draw)
{
    s_BufferManager = buf;
    s_TextureManager = tex;
    s_DrawContext = draw;
}

Mesh::Mesh(std::vector<Vertex> vertices,
           std::vector<unsigned int> indices,
           std::vector<Texture> textures,
           bool setupGPU)
{
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    if (setupGPU)
    {
        setupMesh();
    }

    if (!vertices.empty())
    {
        aabb.minBound = vertices[0].Position;
        aabb.maxBound = vertices[0].Position;
        for (const auto &v : vertices)
        {
            aabb.minBound.x = (std::min)(aabb.minBound.x, v.Position.x);
            aabb.minBound.y = (std::min)(aabb.minBound.y, v.Position.y);
            aabb.minBound.z = (std::min)(aabb.minBound.z, v.Position.z);

            aabb.maxBound.x = (std::max)(aabb.maxBound.x, v.Position.x);
            aabb.maxBound.y = (std::max)(aabb.maxBound.y, v.Position.y);
            aabb.maxBound.z = (std::max)(aabb.maxBound.z, v.Position.z);
        }
    }
}

Mesh::~Mesh()
{
    if (m_Initialized && s_BufferManager)
    {
        try {
            s_BufferManager->DeleteVertexArray(VAO);
            s_BufferManager->DeleteBuffer(VBO);
            s_BufferManager->DeleteBuffer(EBO);
            if (instanceVBO != 0) s_BufferManager->DeleteBuffer(instanceVBO);
        } catch (...) {
            LOGGER_ERROR("Mesh") << "Destructor: CRASH during buffer deletion";
        }
    }
}

void Mesh::Draw(Shader &shader, bool bindTextures)
{
    auto& tm = GetTextureManager();
    auto& dm = GetDrawContext();
    auto& bm = GetBufferManager();

    if (bindTextures)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            tm.ActiveTexture(static_cast<TextureUnit>(i));

            std::string number;
            std::string name = textures[i].type;

            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            std::string fullName = name + number;
            shader.setInt(fullName.c_str(), i);
            shader.setInt(("material." + fullName).c_str(), i);
            
            tm.BindTexture(TextureType::Texture2D, textures[i].id);
        }
    }

    shader.setBool("isInstanced", false);

    static int meshDrawCount = 0;
    meshDrawCount++;
    bool logThisMesh = (meshDrawCount <= 10);

    bm.BindVertexArray(VAO);
    dm.DrawElements(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0);
    bm.BindVertexArray(0);

    tm.ActiveTexture(TextureUnit::Texture0);
}

void Mesh::DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models, bool bindTextures)
{
    auto& tm = GetTextureManager();
    auto& dm = GetDrawContext();
    auto& bm = GetBufferManager();

    if (bindTextures)
    {
        unsigned int diffuseNr = 1;
        unsigned int specularNr = 1;
        unsigned int normalNr = 1;
        unsigned int heightNr = 1;

        for (unsigned int i = 0; i < textures.size(); i++)
        {
            tm.ActiveTexture(static_cast<TextureUnit>(i));

            std::string number;
            std::string name = textures[i].type;

            if (name == "texture_diffuse")
                number = std::to_string(diffuseNr++);
            else if (name == "texture_specular")
                number = std::to_string(specularNr++);
            else if (name == "texture_normal")
                number = std::to_string(normalNr++);
            else if (name == "texture_height")
                number = std::to_string(heightNr++);

            std::string fullName = name + number;
            shader.setInt(fullName.c_str(), i);
            shader.setInt(("material." + fullName).c_str(), i);
            
            tm.BindTexture(TextureType::Texture2D, textures[i].id);
        }
    }

    shader.setBool("isInstanced", true);

    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    bm.BufferData(BufferType::ArrayBuffer, models.size() * sizeof(glm::mat4), models.data(), BufferUsage::DynamicDraw);
    bm.BindBuffer(BufferType::ArrayBuffer, 0);

    bm.BindVertexArray(VAO);
    dm.DrawElementsInstanced(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0, static_cast<int>(models.size()));
    bm.BindVertexArray(0);

    shader.setBool("isInstanced", false);
    tm.ActiveTexture(TextureUnit::Texture0);
}

void Mesh::setupMesh()
{
    if (m_Initialized || !s_BufferManager) return;
    auto& bm = GetBufferManager();

    VAO = bm.CreateVertexArray();
    VBO = bm.CreateBuffer();
    EBO = bm.CreateBuffer();

    bm.BindVertexArray(VAO);

    bm.BindBuffer(BufferType::ArrayBuffer, VBO);
    bm.BufferData(BufferType::ArrayBuffer, vertices.size() * sizeof(Vertex), vertices.data(), BufferUsage::StaticDraw);

    bm.BindBuffer(BufferType::ElementArrayBuffer, EBO);
    bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(), BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, Position));

    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, Normal));

    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, TexCoords));

    bm.EnableVertexAttribArray(3);
    bm.VertexAttribPointer(3, 3, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, Tangent));

    bm.EnableVertexAttribArray(4);
    bm.VertexAttribPointer(4, 3, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, Bitangent));

    bm.EnableVertexAttribArray(5);
    bm.VertexAttribIPointer(5, 4, DataType::Int, sizeof(Vertex), (void *)offsetof(Vertex, m_BoneIDs));

    bm.EnableVertexAttribArray(6);
    bm.VertexAttribPointer(6, 4, DataType::Float, false, sizeof(Vertex), (void *)offsetof(Vertex, m_Weights));

    instanceVBO = bm.CreateBuffer();
    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    glm::mat4 identity(1.0f);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(glm::mat4), &identity, BufferUsage::StaticDraw);

    std::size_t vec4Size = sizeof(glm::vec4);

    bm.EnableVertexAttribArray(10);
    bm.VertexAttribPointer(10, 4, DataType::Float, false, 4 * vec4Size, (void *)0);
    bm.EnableVertexAttribArray(11);
    bm.VertexAttribPointer(11, 4, DataType::Float, false, 4 * vec4Size, (void *)(1 * vec4Size));
    bm.EnableVertexAttribArray(12);
    bm.VertexAttribPointer(12, 4, DataType::Float, false, 4 * vec4Size, (void *)(2 * vec4Size));
    bm.EnableVertexAttribArray(13);
    bm.VertexAttribPointer(13, 4, DataType::Float, false, 4 * vec4Size, (void *)(3 * vec4Size));

    bm.VertexAttribDivisor(10, 1);
    bm.VertexAttribDivisor(11, 1);
    bm.VertexAttribDivisor(12, 1);
    bm.VertexAttribDivisor(13, 1);

    bm.BindVertexArray(0);
    m_Initialized = true;
}
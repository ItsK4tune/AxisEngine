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

Mesh::Mesh(std::vector<uint8_t> vertexData, size_t vertexCount, size_t vertexStride, bool isSkinned, 
           std::vector<unsigned int> indices, std::vector<Texture> textures, bool setupGPU)
    : m_VertexData(std::move(vertexData)), m_VertexCount(vertexCount), m_VertexStride(vertexStride), m_IsSkinned(isSkinned),
      indices(std::move(indices)), textures(std::move(textures)), VAO(0), VBO(0), EBO(0), instanceVBO(0)
{
    if (setupGPU)
    {
        setupMesh();
    }

    if (m_VertexCount > 0)
    {
        // Read the first position to initialize AABB
        const float* firstPos = reinterpret_cast<const float*>(m_VertexData.data());
        aabb.minBound = glm::vec3(firstPos[0], firstPos[1], firstPos[2]);
        aabb.maxBound = aabb.minBound;
        
        for (size_t i = 0; i < m_VertexCount; ++i)
        {
            const float* pos = reinterpret_cast<const float*>(m_VertexData.data() + i * m_VertexStride);
            aabb.minBound.x = (std::min)(aabb.minBound.x, pos[0]);
            aabb.minBound.y = (std::min)(aabb.minBound.y, pos[1]);
            aabb.minBound.z = (std::min)(aabb.minBound.z, pos[2]);

            aabb.maxBound.x = (std::max)(aabb.maxBound.x, pos[0]);
            aabb.maxBound.y = (std::max)(aabb.maxBound.y, pos[1]);
            aabb.maxBound.z = (std::max)(aabb.maxBound.z, pos[2]);
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

            // Build uniform name — these strings are small and SSO-eligible
            const char* prefix = "";
            int num = 0;
            const auto& type = textures[i].type;

            if (type == "texture_diffuse") { prefix = "texture_diffuse"; num = diffuseNr++; }
            else if (type == "texture_specular") { prefix = "texture_specular"; num = specularNr++; }
            else if (type == "texture_normal") { prefix = "texture_normal"; num = normalNr++; }
            else if (type == "texture_height") { prefix = "texture_height"; num = heightNr++; }

            // Use Shader's internal location cache — only does GL lookup once per name
            char fullName[64];
            snprintf(fullName, sizeof(fullName), "%s%d", prefix, num);
            shader.setInt(fullName, i);

            char materialName[80];
            snprintf(materialName, sizeof(materialName), "material.%s%d", prefix, num);
            shader.setInt(materialName, i);
            
            tm.BindTexture(TextureType::Texture2D, textures[i].id);
        }
    }

    shader.setBool("isInstanced", false);



    bm.BindVertexArray(VAO);
    dm.DrawElements(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0);
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

            const char* prefix = "";
            int num = 0;
            const auto& type = textures[i].type;

            if (type == "texture_diffuse") { prefix = "texture_diffuse"; num = diffuseNr++; }
            else if (type == "texture_specular") { prefix = "texture_specular"; num = specularNr++; }
            else if (type == "texture_normal") { prefix = "texture_normal"; num = normalNr++; }
            else if (type == "texture_height") { prefix = "texture_height"; num = heightNr++; }

            char fullName[64];
            snprintf(fullName, sizeof(fullName), "%s%d", prefix, num);
            shader.setInt(fullName, i);

            char materialName[80];
            snprintf(materialName, sizeof(materialName), "material.%s%d", prefix, num);
            shader.setInt(materialName, i);
            
            tm.BindTexture(TextureType::Texture2D, textures[i].id);
        }
    }

    shader.setBool("isInstanced", true);

    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    size_t requiredSize = models.size() * sizeof(glm::mat4);
    if (requiredSize > m_InstanceBufferCapacity)
    {
        m_InstanceBufferCapacity = requiredSize;
        bm.BufferData(BufferType::ArrayBuffer, m_InstanceBufferCapacity, models.data(), BufferUsage::DynamicDraw);
    }
    else
    {
        bm.BufferData(BufferType::ArrayBuffer, m_InstanceBufferCapacity, NULL, BufferUsage::DynamicDraw); // Orphan
        bm.BufferSubData(BufferType::ArrayBuffer, 0, requiredSize, models.data());
    }
    bm.BindBuffer(BufferType::ArrayBuffer, 0);

    bm.BindVertexArray(VAO);
    dm.DrawElementsInstanced(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0, static_cast<int>(models.size()));

    shader.setBool("isInstanced", false);
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
    bm.BufferData(BufferType::ArrayBuffer, m_VertexData.size(), m_VertexData.data(), BufferUsage::StaticDraw);

    bm.BindBuffer(BufferType::ElementArrayBuffer, EBO);
    bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(), BufferUsage::StaticDraw);

    // Positions (12B)
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, m_VertexStride, (void *)0);

    // Normals (12B)
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, m_VertexStride, (void *)12);

    // TexCoords (8B)
    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, m_VertexStride, (void *)24);

    if (m_IsSkinned) {
        // Bones (16B)
        bm.EnableVertexAttribArray(5);
        bm.VertexAttribIPointer(5, 4, DataType::Int, m_VertexStride, (void *)32);

        // Weights (16B)
        bm.EnableVertexAttribArray(6);
        bm.VertexAttribPointer(6, 4, DataType::Float, false, m_VertexStride, (void *)48);
    }

    instanceVBO = bm.CreateBuffer();
    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    glm::mat4 identity(1.0f);
    bm.BufferData(BufferType::ArrayBuffer, sizeof(glm::mat4), &identity, BufferUsage::StaticDraw);
    m_InstanceBufferCapacity = sizeof(glm::mat4);

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
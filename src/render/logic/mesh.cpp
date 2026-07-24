#include <algorithm>
#include <limits>
#include <utility>

#include <resource/unit/mesh.h>
#include <cstring>
#include <cstddef>
#include <core/logic/logger.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/transient_buffer_ring.h>
#include <render/type/graphics_types.h>
#include <glm/gtc/matrix_transform.hpp>

IBufferManager* Mesh::s_BufferManager = nullptr;
ITextureManager* Mesh::s_TextureManager = nullptr;
IDrawContext* Mesh::s_DrawContext = nullptr;

namespace
{
struct MeshTextureSlot
{
    const char* legacyPrefix = "";
    const char* canonicalUniform = nullptr;
    unsigned int canonicalUnit = 0;
    bool hasCanonicalUnit = false;
};

MeshTextureSlot ResolveTextureSlot(const std::string& type)
{
    if (type == "texture_diffuse")
        return {"texture_diffuse", "u_AlbedoMap", 0, true};
    if (type == "texture_normal")
        return {"texture_normal", "u_NormalMap", 1, true};
    if (type == "texture_metallic")
        return {"texture_metallic", "u_MetallicMap", 2, true};
    if (type == "texture_roughness")
        return {"texture_roughness", "u_RoughnessMap", 3, true};
    if (type == "texture_ao")
        return {"texture_ao", "u_AOMap", 4, true};
    if (type == "texture_emissive")
        return {"texture_emissive", "u_EmissiveMap", 5, true};
    if (type == "texture_specular")
        return {"texture_specular", "u_SpecularMap", 6, true};
    if (type == "texture_height")
        return {"texture_height", "u_HeightMap", 0, false};

    return {};
}

unsigned int IncrementTextureCounter(const std::string& type, unsigned int& diffuseNr, unsigned int& specularNr,
                                     unsigned int& normalNr, unsigned int& heightNr, unsigned int& metallicNr,
                                     unsigned int& roughnessNr, unsigned int& aoNr, unsigned int& emissiveNr)
{
    if (type == "texture_diffuse")
        return diffuseNr++;
    if (type == "texture_specular")
        return specularNr++;
    if (type == "texture_normal")
        return normalNr++;
    if (type == "texture_height")
        return heightNr++;
    if (type == "texture_metallic")
        return metallicNr++;
    if (type == "texture_roughness")
        return roughnessNr++;
    if (type == "texture_ao")
        return aoNr++;
    if (type == "texture_emissive")
        return emissiveNr++;

    return 1;
}

void BindMeshTextures(Shader& shader, const std::vector<Texture>& textures, ITextureManager& textureManager)
{
    unsigned int diffuseNr = 1;
    unsigned int specularNr = 1;
    unsigned int normalNr = 1;
    unsigned int heightNr = 1;
    unsigned int metallicNr = 1;
    unsigned int roughnessNr = 1;
    unsigned int aoNr = 1;
    unsigned int emissiveNr = 1;
    unsigned int fallbackUnit = 7;

    for (const auto& texture : textures)
    {
        const auto slot = ResolveTextureSlot(texture.type);
        const unsigned int occurrence = IncrementTextureCounter(texture.type, diffuseNr, specularNr, normalNr, heightNr,
                                                                metallicNr, roughnessNr, aoNr, emissiveNr);
        const unsigned int unit = (slot.hasCanonicalUnit && occurrence == 1) ? slot.canonicalUnit : fallbackUnit++;

        textureManager.ActiveTexture(static_cast<TextureUnit>(unit));

        if (slot.legacyPrefix[0] != '\0')
        {
            char fullName[64];
            snprintf(fullName, sizeof(fullName), "%s%d", slot.legacyPrefix, occurrence);
            shader.setInt(fullName, static_cast<int>(unit));

            char materialName[80];
            snprintf(materialName, sizeof(materialName), "material.%s%d", slot.legacyPrefix, occurrence);
            shader.setInt(materialName, static_cast<int>(unit));
        }

        if (slot.canonicalUniform && occurrence == 1)
            shader.setInt(slot.canonicalUniform, static_cast<int>(unit));

        textureManager.BindTexture(TextureType::Texture2D, texture.id);
    }
}
}  // namespace

void Mesh::SetManagers(IBufferManager* buf, ITextureManager* tex, IDrawContext* draw)
{
    s_BufferManager = buf;
    s_TextureManager = tex;
    s_DrawContext = draw;
}

Mesh::Mesh(std::vector<uint8_t> vertexData, size_t vertexCount, size_t vertexStride, bool isSkinned,
           std::vector<unsigned int> indices, std::vector<Texture> textures, bool setupGPU)
    : m_VertexData(std::move(vertexData)),
      m_VertexCount(vertexCount),
      m_VertexStride(vertexStride),
      m_IsSkinned(isSkinned),
      indices(std::move(indices)),
      textures(std::move(textures)),
      VAO(0),
      VBO(0),
      EBO(0),
      instanceVBO(0)
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
    ReleaseGpuResources();
}

Mesh::Mesh(Mesh&& other) noexcept
    : m_VertexData(std::move(other.m_VertexData)),
      m_VertexCount(other.m_VertexCount),
      m_VertexStride(other.m_VertexStride),
      m_IsSkinned(other.m_IsSkinned),
      indices(std::move(other.indices)),
      textures(std::move(other.textures)),
      VAO(other.VAO),
      aabb(other.aabb),
      VBO(other.VBO),
      EBO(other.EBO),
      instanceVBO(other.instanceVBO),
      m_Initialized(other.m_Initialized),
      m_InstanceBufferCapacity(other.m_InstanceBufferCapacity),
      m_InstanceUpload(std::move(other.m_InstanceUpload)),
      m_CompactPositions(std::move(other.m_CompactPositions))
{
    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.instanceVBO = 0;
    other.m_Initialized = false;
    other.m_InstanceBufferCapacity = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
    if (this == &other)
        return *this;
    ReleaseGpuResources();
    m_VertexData = std::move(other.m_VertexData);
    m_VertexCount = other.m_VertexCount;
    m_VertexStride = other.m_VertexStride;
    m_IsSkinned = other.m_IsSkinned;
    indices = std::move(other.indices);
    textures = std::move(other.textures);
    VAO = other.VAO;
    aabb = other.aabb;
    VBO = other.VBO;
    EBO = other.EBO;
    instanceVBO = other.instanceVBO;
    m_Initialized = other.m_Initialized;
    m_InstanceBufferCapacity = other.m_InstanceBufferCapacity;
    m_InstanceUpload = std::move(other.m_InstanceUpload);
    m_CompactPositions = std::move(other.m_CompactPositions);

    other.VAO = 0;
    other.VBO = 0;
    other.EBO = 0;
    other.instanceVBO = 0;
    other.m_Initialized = false;
    other.m_InstanceBufferCapacity = 0;
    return *this;
}

void Mesh::ReleaseGpuResources() noexcept
{
    m_InstanceUpload.reset();
    if (m_Initialized && s_BufferManager)
    {
        try
        {
            s_BufferManager->DeleteVertexArray(VAO);
            s_BufferManager->DeleteBuffer(VBO);
            s_BufferManager->DeleteBuffer(EBO);
        }
        catch (...)
        {
            LOGGER_ERROR("Mesh") << "Destructor: CRASH during buffer deletion";
        }
    }
    VAO = VBO = EBO = instanceVBO = 0;
    m_Initialized = false;
    m_InstanceBufferCapacity = 0;
}

glm::vec3 Mesh::GetPosition(size_t vertexIndex) const
{
    if (vertexIndex >= m_VertexCount)
        return glm::vec3(0.0f);
    if (!m_VertexData.empty())
    {
        float value[3]{};
        if (!CopyCpuVertex(vertexIndex, value, sizeof(value)))
            return glm::vec3(0.0f);
        return glm::vec3(value[0], value[1], value[2]);
    }
    return vertexIndex < m_CompactPositions.size() ? m_CompactPositions[vertexIndex] : glm::vec3(0.0f);
}

bool Mesh::CopyCpuVertex(size_t vertexIndex, void* destination, size_t byteCount) const
{
    if (!destination || byteCount == 0 || vertexIndex >= m_VertexCount || m_VertexStride < byteCount)
        return false;
    if (vertexIndex > (std::numeric_limits<size_t>::max)() / m_VertexStride)
        return false;

    const size_t offset = vertexIndex * m_VertexStride;
    if (offset > m_VertexData.size() || byteCount > m_VertexData.size() - offset)
        return false;

    std::memcpy(destination, m_VertexData.data() + offset, byteCount);
    return true;
}

void Mesh::ReleaseCpuVertexData()
{
    if (m_VertexData.empty())
        return;
    m_CompactPositions.resize(m_VertexCount);
    for (size_t index = 0; index < m_VertexCount; ++index)
        m_CompactPositions[index] = GetPosition(index);
    std::vector<uint8_t>().swap(m_VertexData);
}

void Mesh::Draw(Shader& shader, bool bindTextures)
{
    auto& tm = GetTextureManager();
    auto& dm = GetDrawContext();
    auto& bm = GetBufferManager();

    if (bindTextures)
    {
        BindMeshTextures(shader, textures, tm);
    }

    shader.setBool_Fast(shader.m_Loc_u_IsInstanced, "u_IsInstanced", false);

    bm.BindVertexArray(VAO);
    dm.DrawElements(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0);
}

void Mesh::DrawInstanced(Shader& shader, const std::vector<MeshInstanceData>& instances, bool bindTextures)
{
    if (instances.empty() || !m_InstanceUpload)
        return;
    auto& tm = GetTextureManager();
    auto& dm = GetDrawContext();
    auto& bm = GetBufferManager();

    if (bindTextures)
    {
        BindMeshTextures(shader, textures, tm);
    }

    shader.setBool_Fast(shader.m_Loc_u_IsInstanced, "u_IsInstanced", true);

    size_t requiredSize = instances.size() * sizeof(MeshInstanceData);
    const auto slice = m_InstanceUpload->Upload(instances.data(), requiredSize);
    instanceVBO = slice.buffer;
    m_InstanceBufferCapacity = m_InstanceUpload->GetSegmentCapacity();
    bm.BindVertexArray(VAO);
    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    const size_t vec4Size = sizeof(glm::vec4);
    for (unsigned int column = 0; column < 4; ++column)
        bm.VertexAttribPointer(10 + column, 4, DataType::Float, false, sizeof(MeshInstanceData),
                               reinterpret_cast<void*>(slice.offset + offsetof(MeshInstanceData, model) +
                                                       column * vec4Size));
    bm.VertexAttribIPointer(14, 1, DataType::UnsignedInt, sizeof(MeshInstanceData),
                            reinterpret_cast<void*>(slice.offset + offsetof(MeshInstanceData, entityId)));
    dm.DrawElementsInstanced(Primitive::Triangles, static_cast<int>(indices.size()), DataType::UnsignedInt, 0,
                             static_cast<int>(instances.size()));
    m_InstanceUpload->Commit();

    shader.setBool_Fast(shader.m_Loc_u_IsInstanced, "u_IsInstanced", false);
}

void Mesh::setupMesh()
{
    if (m_Initialized || !s_BufferManager)
        return;
    auto& bm = GetBufferManager();

    VAO = bm.CreateVertexArray();
    VBO = bm.CreateBuffer();
    EBO = bm.CreateBuffer();

    bm.BindVertexArray(VAO);

    bm.BindBuffer(BufferType::ArrayBuffer, VBO);
    bm.BufferData(BufferType::ArrayBuffer, m_VertexData.size(), m_VertexData.data(), BufferUsage::StaticDraw);

    bm.BindBuffer(BufferType::ElementArrayBuffer, EBO);
    bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(),
                  BufferUsage::StaticDraw);

    // Positions (12B)
    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, m_VertexStride, (void*)0);

    // Normals (12B)
    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, m_VertexStride, (void*)12);

    // TexCoords (8B)
    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, m_VertexStride, (void*)24);

    if (m_IsSkinned)
    {
        // Bones (16B)
        bm.EnableVertexAttribArray(5);
        bm.VertexAttribIPointer(5, 4, DataType::Int, m_VertexStride, (void*)32);

        // Weights (16B)
        bm.EnableVertexAttribArray(6);
        bm.VertexAttribPointer(6, 4, DataType::Float, false, m_VertexStride, (void*)48);
    }

    m_InstanceUpload = std::make_unique<TransientBufferRing>();
    m_InstanceUpload->Initialize(bm, BufferType::ArrayBuffer, sizeof(MeshInstanceData) * 64);
    instanceVBO = m_InstanceUpload->GetBuffer();
    bm.BindBuffer(BufferType::ArrayBuffer, instanceVBO);
    m_InstanceBufferCapacity = m_InstanceUpload->GetSegmentCapacity();

    std::size_t vec4Size = sizeof(glm::vec4);

    bm.EnableVertexAttribArray(10);
    bm.VertexAttribPointer(10, 4, DataType::Float, false, sizeof(MeshInstanceData),
                           (void*)(offsetof(MeshInstanceData, model) + 0 * vec4Size));
    bm.EnableVertexAttribArray(11);
    bm.VertexAttribPointer(11, 4, DataType::Float, false, sizeof(MeshInstanceData),
                           (void*)(offsetof(MeshInstanceData, model) + 1 * vec4Size));
    bm.EnableVertexAttribArray(12);
    bm.VertexAttribPointer(12, 4, DataType::Float, false, sizeof(MeshInstanceData),
                           (void*)(offsetof(MeshInstanceData, model) + 2 * vec4Size));
    bm.EnableVertexAttribArray(13);
    bm.VertexAttribPointer(13, 4, DataType::Float, false, sizeof(MeshInstanceData),
                           (void*)(offsetof(MeshInstanceData, model) + 3 * vec4Size));

    bm.EnableVertexAttribArray(14);
    bm.VertexAttribIPointer(14, 1, DataType::UnsignedInt, sizeof(MeshInstanceData),
                            (void*)offsetof(MeshInstanceData, entityId));

    bm.VertexAttribDivisor(10, 1);
    bm.VertexAttribDivisor(11, 1);
    bm.VertexAttribDivisor(12, 1);
    bm.VertexAttribDivisor(13, 1);
    bm.VertexAttribDivisor(14, 1);

    bm.BindVertexArray(0);
    m_Initialized = true;
}

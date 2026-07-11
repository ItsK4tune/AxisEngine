#include <render/logic/static_batch_manager.h>
#include <core/logic/logger.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <cstdint>
#include <fstream>
#include <limits>

IBufferManager* StaticBatchManager::s_BufferManager = nullptr;
IDrawContext* StaticBatchManager::s_DrawContext = nullptr;

namespace
{
constexpr uint32_t BATCH_FILE_MAGIC = 0x48435442;
constexpr uint32_t BATCH_FILE_VERSION = 2;

struct BatchFileHeader
{
    uint32_t magic = BATCH_FILE_MAGIC;
    uint32_t version = BATCH_FILE_VERSION;
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
};

template <typename T>
bool ReadScalar(std::istream& file, T& value)
{
    file.read(reinterpret_cast<char*>(&value), sizeof(T));
    return file.good();
}

template <typename T>
void WriteScalar(std::ostream& file, T value)
{
    file.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

bool ReadHeader(std::istream& file, BatchFileHeader& header)
{
    return ReadScalar(file, header.magic) && ReadScalar(file, header.version) && ReadScalar(file, header.vertexCount) &&
           ReadScalar(file, header.indexCount);
}

void WriteHeader(std::ostream& file, const BatchFileHeader& header)
{
    WriteScalar(file, header.magic);
    WriteScalar(file, header.version);
    WriteScalar(file, header.vertexCount);
    WriteScalar(file, header.indexCount);
}

void WriteVertex(std::ostream& file, const StaticVertex& vertex)
{
    WriteScalar(file, vertex.Position.x);
    WriteScalar(file, vertex.Position.y);
    WriteScalar(file, vertex.Position.z);
    WriteScalar(file, vertex.Normal.x);
    WriteScalar(file, vertex.Normal.y);
    WriteScalar(file, vertex.Normal.z);
    WriteScalar(file, vertex.TexCoords.x);
    WriteScalar(file, vertex.TexCoords.y);
}

bool ReadVertex(std::istream& file, StaticVertex& vertex)
{
    return ReadScalar(file, vertex.Position.x) && ReadScalar(file, vertex.Position.y) &&
           ReadScalar(file, vertex.Position.z) && ReadScalar(file, vertex.Normal.x) &&
           ReadScalar(file, vertex.Normal.y) && ReadScalar(file, vertex.Normal.z) &&
           ReadScalar(file, vertex.TexCoords.x) && ReadScalar(file, vertex.TexCoords.y);
}

bool ReadPortableBatchData(std::istream& file, const BatchFileHeader& header, std::vector<StaticVertex>& vertices,
                           std::vector<unsigned int>& indices)
{
    vertices.resize(header.vertexCount);
    indices.resize(header.indexCount);

    for (auto& vertex : vertices)
    {
        if (!ReadVertex(file, vertex))
            return false;
    }

    for (auto& index : indices)
    {
        uint32_t rawIndex = 0;
        if (!ReadScalar(file, rawIndex))
            return false;
        index = static_cast<unsigned int>(rawIndex);
    }

    return true;
}

bool ReadLegacyBatchData(std::istream& file, const BatchFileHeader& header, std::vector<StaticVertex>& vertices,
                         std::vector<unsigned int>& indices)
{
    vertices.resize(header.vertexCount);
    indices.resize(header.indexCount);
    file.read(reinterpret_cast<char*>(vertices.data()), static_cast<std::streamsize>(vertices.size() * sizeof(StaticVertex)));
    file.read(reinterpret_cast<char*>(indices.data()), static_cast<std::streamsize>(indices.size() * sizeof(unsigned int)));
    return file.good();
}
} // namespace

void StaticBatchManager::SetManagers(IBufferManager& bufferManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_DrawContext = &drawContext;
}

IBufferManager& StaticBatchManager::GetBufferManager()
{
    if (!s_BufferManager)
    {
        LOGGER_ERROR("StaticBatchManager") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in StaticBatchManager");
    }
    return *s_BufferManager;
}

IDrawContext& StaticBatchManager::GetDrawContext()
{
    if (!s_DrawContext)
    {
        LOGGER_ERROR("StaticBatchManager") << "DrawContext not set!";
        throw std::runtime_error("DrawContext not set in StaticBatchManager");
    }
    return *s_DrawContext;
}

StaticBatchManager::StaticBatchManager()
{
}

StaticBatchManager::~StaticBatchManager()
{
    Clear();
}

void StaticBatchManager::CreateBatch(const std::string& name, const std::vector<std::shared_ptr<Model>>& models,
                                     const std::vector<glm::mat4>& transforms)
{
    if (models.size() != transforms.size())
    {
        LOGGER_ERROR("StaticBatchManager") << "Model and transform count mismatch";
        return;
    }

    std::vector<StaticVertex> mergedVertices;
    std::vector<unsigned int> mergedIndices;

    MergeMeshes(models, transforms, mergedVertices, mergedIndices);

    BatchData batch;
    batch.vertices = mergedVertices;
    batch.indices = mergedIndices;
    CreateGPUBuffers(batch, mergedVertices, mergedIndices);
    batch.shader = nullptr;
    batch.materialName = "";

    m_Batches[name] = batch;

    LOGGER_INFO("StaticBatchManager") << "Created batch: " << name << " (" << mergedVertices.size() << " vertices, "
                                      << mergedIndices.size() << " indices)";
}

void StaticBatchManager::MergeMeshes(const std::vector<std::shared_ptr<Model>>& models,
                                     const std::vector<glm::mat4>& transforms, std::vector<StaticVertex>& outVertices,
                                     std::vector<unsigned int>& outIndices)
{
    unsigned int indexOffset = 0;

    for (size_t i = 0; i < models.size(); i++)
    {
        const glm::mat4& transform = transforms[i];
        std::shared_ptr<Model> model = models[i];

        for (const auto& mesh : model->meshes)
        {
            if (mesh.m_IsSkinned)
            {
                LOGGER_WARN("StaticBatchManager") << "Attempted to static-batch a skinned mesh. Skipping.";
                continue;
            }

            glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
            const StaticVertex* staticVerts = reinterpret_cast<const StaticVertex*>(mesh.m_VertexData.data());

            for (size_t vIdx = 0; vIdx < mesh.m_VertexCount; vIdx++)
            {
                const StaticVertex& v = staticVerts[vIdx];
                StaticVertex transformedVertex = v;

                glm::vec4 transformedPos = transform * glm::vec4(v.Position, 1.0f);
                transformedVertex.Position = glm::vec3(transformedPos);
                transformedVertex.Normal = normalMatrix * v.Normal;

                outVertices.push_back(transformedVertex);
            }

            for (unsigned int index : mesh.indices)
            {
                outIndices.push_back(index + indexOffset);
            }

            indexOffset += mesh.m_VertexCount;
        }
    }
}

void StaticBatchManager::CreateGPUBuffers(BatchData& batch, const std::vector<StaticVertex>& vertices,
                                          const std::vector<unsigned int>& indices)
{
    if (!s_BufferManager)
        return;
    auto& bm = GetBufferManager();

    batch.vertexCount = vertices.size();
    batch.indexCount = indices.size();

    batch.vao = bm.GenVertexArray();
    batch.vbo = bm.GenBuffer();
    batch.ebo = bm.GenBuffer();

    bm.BindVertexArray(batch.vao);

    bm.BindBuffer(BufferType::ArrayBuffer, batch.vbo);
    bm.BufferData(BufferType::ArrayBuffer, vertices.size() * sizeof(StaticVertex), vertices.data(),
                  BufferUsage::StaticDraw);

    bm.BindBuffer(BufferType::ElementArrayBuffer, batch.ebo);
    bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(),
                  BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(StaticVertex), (void*)0);

    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Normal));

    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, sizeof(StaticVertex),
                           (void*)offsetof(StaticVertex, TexCoords));

    bm.BindVertexArray(0);
}

void StaticBatchManager::RenderBatch(const std::string& name)
{
    if (!s_BufferManager || !s_DrawContext)
        return;

    auto it = m_Batches.find(name);
    if (it == m_Batches.end())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch not found: " << name;
        return;
    }

    const BatchData& batch = it->second;

    GetBufferManager().BindVertexArray(batch.vao);
    GetDrawContext().DrawElements(Primitive::Triangles, static_cast<unsigned int>(batch.indexCount),
                                  DataType::UnsignedInt, 0);
    GetBufferManager().BindVertexArray(0);
}

void StaticBatchManager::RenderAllBatches()
{
    if (!s_BufferManager || !s_DrawContext)
        return;
    for (const auto& [name, batch] : m_Batches)
    {
        GetBufferManager().BindVertexArray(batch.vao);
        GetDrawContext().DrawElements(Primitive::Triangles, static_cast<unsigned int>(batch.indexCount),
                                      DataType::UnsignedInt, 0);
    }
    GetBufferManager().BindVertexArray(0);
}

bool StaticBatchManager::LoadBatchFromFile(const std::string& name, const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOGGER_ERROR("StaticBatchManager") << "Failed to open batch file: " << path;
        return false;
    }

    BatchFileHeader header;
    if (!ReadHeader(file, header))
    {
        LOGGER_ERROR("StaticBatchManager") << "Truncated batch file header: " << path;
        return false;
    }

    if (header.magic != BATCH_FILE_MAGIC)
    {
        LOGGER_ERROR("StaticBatchManager") << "Invalid batch file magic";
        return false;
    }

    if (header.version != 1 && header.version != BATCH_FILE_VERSION)
    {
        LOGGER_ERROR("StaticBatchManager") << "Unsupported batch file version: " << header.version;
        return false;
    }

    if (header.indexCount > static_cast<uint32_t>((std::numeric_limits<unsigned int>::max)()))
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch file index count is too large";
        return false;
    }

    std::vector<StaticVertex> vertices;
    std::vector<unsigned int> indices;
    bool loaded = header.version == BATCH_FILE_VERSION ? ReadPortableBatchData(file, header, vertices, indices)
                                                       : ReadLegacyBatchData(file, header, vertices, indices);
    if (!loaded)
    {
        LOGGER_ERROR("StaticBatchManager") << "Truncated batch file data: " << path;
        return false;
    }

    BatchData batch;
    batch.vertices = vertices;
    batch.indices = indices;
    CreateGPUBuffers(batch, vertices, indices);
    batch.shader = nullptr;
    batch.materialName = "";

    m_Batches[name] = batch;

    LOGGER_INFO("StaticBatchManager") << "Loaded batch from file: " << name;
    return true;
}

void StaticBatchManager::SaveBatchToFile(const std::string& name, const std::string& path)
{
    auto it = m_Batches.find(name);
    if (it == m_Batches.end())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch not found: " << name;
        return;
    }

    const BatchData& batch = it->second;
    if (batch.vertices.empty() || batch.indices.empty())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch has no CPU-side mesh data to save: " << name;
        return;
    }

    if (batch.vertices.size() > (std::numeric_limits<uint32_t>::max)() ||
        batch.indices.size() > (std::numeric_limits<uint32_t>::max)())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch is too large to save: " << name;
        return;
    }

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open())
    {
        LOGGER_ERROR("StaticBatchManager") << "Failed to open batch file for writing: " << path;
        return;
    }

    BatchFileHeader header;
    header.vertexCount = static_cast<uint32_t>(batch.vertices.size());
    header.indexCount = static_cast<uint32_t>(batch.indices.size());

    WriteHeader(file, header);
    for (const auto& vertex : batch.vertices)
    {
        WriteVertex(file, vertex);
    }
    for (unsigned int index : batch.indices)
    {
        WriteScalar(file, static_cast<uint32_t>(index));
    }

    if (!file.good())
    {
        LOGGER_ERROR("StaticBatchManager") << "Failed while writing batch file: " << path;
        return;
    }

    LOGGER_INFO("StaticBatchManager") << "Saved batch to file: " << path;
}

void StaticBatchManager::Clear()
{
    if (s_BufferManager)
    {
        for (auto& [name, batch] : m_Batches)
        {
            s_BufferManager->DeleteVertexArrays(1, &batch.vao);
            s_BufferManager->DeleteBuffers(1, &batch.vbo);
            s_BufferManager->DeleteBuffers(1, &batch.ebo);
        }
    }
    m_Batches.clear();
}

bool StaticBatchManager::HasBatch(const std::string& name) const
{
    return m_Batches.find(name) != m_Batches.end();
}

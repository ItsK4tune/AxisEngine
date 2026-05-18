#include <fstream>
#include <render/logic/static_batch_manager.h>
#include <render/interface/i_buffer_manager.h>
#include <render/interface/i_draw_context.h>
#include <core/logic/logger.h>

IBufferManager* StaticBatchManager::s_BufferManager = nullptr;
IDrawContext* StaticBatchManager::s_DrawContext = nullptr;

void StaticBatchManager::SetManagers(IBufferManager& bufferManager, IDrawContext& drawContext)
{
    s_BufferManager = &bufferManager;
    s_DrawContext = &drawContext;
}

IBufferManager& StaticBatchManager::GetBufferManager()
{
    if (!s_BufferManager) {
        LOGGER_ERROR("StaticBatchManager") << "BufferManager not set!";
        throw std::runtime_error("BufferManager not set in StaticBatchManager");
    }
    return *s_BufferManager;
}

IDrawContext& StaticBatchManager::GetDrawContext()
{
    if (!s_DrawContext) {
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
    CreateGPUBuffers(batch, mergedVertices, mergedIndices);
    batch.shader = nullptr;
    batch.materialName = "";

    m_Batches[name] = batch;

    LOGGER_INFO("StaticBatchManager") << "Created batch: " << name
              << " (" << mergedVertices.size() << " vertices, "
              << mergedIndices.size() << " indices)";
}

void StaticBatchManager::MergeMeshes(const std::vector<std::shared_ptr<Model>>& models,
                                     const std::vector<glm::mat4>& transforms,
                                     std::vector<StaticVertex>& outVertices,
                                     std::vector<unsigned int>& outIndices)
{
    unsigned int indexOffset = 0;

    for (size_t i = 0; i < models.size(); i++)
    {
        const glm::mat4& transform = transforms[i];
        std::shared_ptr<Model> model = models[i];

        for (const auto& mesh : model->meshes)
        {
            if (mesh.m_IsSkinned) {
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
    if (!s_BufferManager) return;
    auto& bm = GetBufferManager();

    batch.vertexCount = vertices.size();
    batch.indexCount = indices.size();

    batch.vao = bm.GenVertexArray();
    batch.vbo = bm.GenBuffer();
    batch.ebo = bm.GenBuffer();

    bm.BindVertexArray(batch.vao);

    bm.BindBuffer(BufferType::ArrayBuffer, batch.vbo);
    bm.BufferData(BufferType::ArrayBuffer, vertices.size() * sizeof(StaticVertex), vertices.data(), BufferUsage::StaticDraw);

    bm.BindBuffer(BufferType::ElementArrayBuffer, batch.ebo);
    bm.BufferData(BufferType::ElementArrayBuffer, indices.size() * sizeof(unsigned int), indices.data(), BufferUsage::StaticDraw);

    bm.EnableVertexAttribArray(0);
    bm.VertexAttribPointer(0, 3, DataType::Float, false, sizeof(StaticVertex), (void*)0);

    bm.EnableVertexAttribArray(1);
    bm.VertexAttribPointer(1, 3, DataType::Float, false, sizeof(StaticVertex), (void*)offsetof(StaticVertex, Normal));

    bm.EnableVertexAttribArray(2);
    bm.VertexAttribPointer(2, 2, DataType::Float, false, sizeof(StaticVertex), (void*)offsetof(StaticVertex, TexCoords));

    bm.BindVertexArray(0);
}

void StaticBatchManager::RenderBatch(const std::string& name)
{
    if (!s_BufferManager || !s_DrawContext) return;

    auto it = m_Batches.find(name);
    if (it == m_Batches.end())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch not found: " << name;
        return;
    }

    const BatchData& batch = it->second;

    GetBufferManager().BindVertexArray(batch.vao);
    GetDrawContext().DrawElements(Primitive::Triangles, static_cast<unsigned int>(batch.indexCount), DataType::UnsignedInt, 0);
    GetBufferManager().BindVertexArray(0);
}

void StaticBatchManager::RenderAllBatches()
{
    if (!s_BufferManager || !s_DrawContext) return;
    for (const auto& [name, batch] : m_Batches)
    {
        GetBufferManager().BindVertexArray(batch.vao);
        GetDrawContext().DrawElements(Primitive::Triangles, static_cast<unsigned int>(batch.indexCount), DataType::UnsignedInt, 0);
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

    struct BatchFileHeader
    {
        uint32_t magic;
        uint32_t version;
        uint32_t vertexCount;
        uint32_t indexCount;
    };

    BatchFileHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(BatchFileHeader));

    if (header.magic != 0x48435442)
    {
        LOGGER_ERROR("StaticBatchManager") << "Invalid batch file magic";
        return false;
    }

    std::vector<StaticVertex> vertices(header.vertexCount);
    std::vector<unsigned int> indices(header.indexCount);

    file.read(reinterpret_cast<char*>(vertices.data()), header.vertexCount * sizeof(StaticVertex));
    file.read(reinterpret_cast<char*>(indices.data()), header.indexCount * sizeof(unsigned int));

    file.close();

    BatchData batch;
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

    LOGGER_WARN("StaticBatchManager") << "Batch saving not yet fully implemented";
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
#include <graphic/geometry/static_batch_manager.h>
#include <utils/logger.h>
#include <fstream>

StaticBatchManager::StaticBatchManager()
{
}

StaticBatchManager::~StaticBatchManager()
{
    Clear();
}

void StaticBatchManager::CreateBatch(const std::string& name, const std::vector<Model*>& models,
                                     const std::vector<glm::mat4>& transforms)
{
    if (models.size() != transforms.size())
    {
        LOGGER_ERROR("StaticBatchManager") << "Model and transform count mismatch";
        return;
    }
    
    std::vector<Vertex> mergedVertices;
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

void StaticBatchManager::MergeMeshes(const std::vector<Model*>& models,
                                     const std::vector<glm::mat4>& transforms,
                                     std::vector<Vertex>& outVertices,
                                     std::vector<unsigned int>& outIndices)
{
    unsigned int indexOffset = 0;
    
    for (size_t i = 0; i < models.size(); i++)
    {
        const glm::mat4& transform = transforms[i];
        Model* model = models[i];
        
        for (const auto& mesh : model->meshes)
        {
            for (const auto& vertex : mesh.vertices)
            {
                Vertex transformedVertex = vertex;
                
                glm::vec4 transformedPos = transform * glm::vec4(vertex.Position, 1.0f);
                transformedVertex.Position = glm::vec3(transformedPos);
                
                glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
                transformedVertex.Normal = normalMatrix * vertex.Normal;
                transformedVertex.Tangent = normalMatrix * vertex.Tangent;
                transformedVertex.Bitangent = normalMatrix * vertex.Bitangent;
                
                outVertices.push_back(transformedVertex);
            }
            
            for (unsigned int index : mesh.indices)
            {
                outIndices.push_back(index + indexOffset);
            }
            
            indexOffset += mesh.vertices.size();
        }
    }
}

void StaticBatchManager::CreateGPUBuffers(BatchData& batch, const std::vector<Vertex>& vertices,
                                         const std::vector<unsigned int>& indices)
{
    batch.vertexCount = vertices.size();
    batch.indexCount = indices.size();
    
    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glGenBuffers(1, &batch.ebo);
    
    glBindVertexArray(batch.vao);
    
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Tangent));
    
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Bitangent));
    
    glBindVertexArray(0);
}

void StaticBatchManager::RenderBatch(const std::string& name)
{
    auto it = m_Batches.find(name);
    if (it == m_Batches.end())
    {
        LOGGER_ERROR("StaticBatchManager") << "Batch not found: " << name;
        return;
    }
    
    const BatchData& batch = it->second;
    
    glBindVertexArray(batch.vao);
    glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void StaticBatchManager::RenderAllBatches()
{
    for (const auto& [name, batch] : m_Batches)
    {
        glBindVertexArray(batch.vao);
        glDrawElements(GL_TRIANGLES, batch.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);
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
    
    std::vector<Vertex> vertices(header.vertexCount);
    std::vector<unsigned int> indices(header.indexCount);
    
    file.read(reinterpret_cast<char*>(vertices.data()), vertices.size() * sizeof(Vertex));
    file.read(reinterpret_cast<char*>(indices.data()), indices.size() * sizeof(unsigned int));
    
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
    for (auto& [name, batch] : m_Batches)
    {
        glDeleteVertexArrays(1, &batch.vao);
        glDeleteBuffers(1, &batch.vbo);
        glDeleteBuffers(1, &batch.ebo);
    }
    m_Batches.clear();
}

bool StaticBatchManager::HasBatch(const std::string& name) const
{
    return m_Batches.find(name) != m_Batches.end();
}

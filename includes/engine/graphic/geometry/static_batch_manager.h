#pragma once

#include <graphic/geometry/model.h>
#include <graphic/core/shader.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

class IBufferManager;
class IDrawContext;

class StaticBatchManager
{
public:
    struct BatchData
    {
        unsigned int vao, vbo, ebo;
        size_t vertexCount;
        size_t indexCount;
        Shader* shader;
        std::string materialName;
    };

    StaticBatchManager();
    ~StaticBatchManager();

    void CreateBatch(const std::string& name, const std::vector<std::shared_ptr<Model>>& models,
                     const std::vector<glm::mat4>& transforms);
    
    void RenderBatch(const std::string& name);
    void RenderAllBatches();
    
    bool LoadBatchFromFile(const std::string& name, const std::string& path);
    void SaveBatchToFile(const std::string& name, const std::string& path);
    
    void Clear();
    bool HasBatch(const std::string& name) const;

    static void SetManagers(IBufferManager& bufferManager, IDrawContext& drawContext);

private:
    std::map<std::string, BatchData> m_Batches;
    
    void MergeMeshes(const std::vector<std::shared_ptr<Model>>& models,
                     const std::vector<glm::mat4>& transforms,
                     std::vector<Vertex>& outVertices,
                     std::vector<unsigned int>& outIndices);
    
    void CreateGPUBuffers(BatchData& batch, const std::vector<Vertex>& vertices,
                         const std::vector<unsigned int>& indices);

    static IBufferManager* s_BufferManager;
    static IDrawContext* s_DrawContext;

    static IBufferManager& GetBufferManager();
    static IDrawContext& GetDrawContext();
};

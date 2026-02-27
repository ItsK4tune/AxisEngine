#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include <graphic/geometry/mesh.h>
#include <graphic/core/shader.h>
#include <graphic/geometry/animdata.h>
#include <math/aabb.h>

class Model
{
public:
    Model() = default;
    Model(std::string const &path, bool isStatic = false, bool gamma = false);
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    void LoadCPU(std::string const& path, bool isStatic = false, bool gamma = false);

    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection = false;
    AABB aabb;

    void Draw(Shader &shader);
    void DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models);

    std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap();
    int &GetBoneCount();

    void AddTexture(const Texture& tex) { textures_loaded.push_back(tex); }

    void UploadToGPU();
    bool IsReadyToRender() const { return m_ReadyToRender; }
    bool IsStatic() const { return m_IsStatic; }
    void SetStatic(bool isStatic) { m_IsStatic = isStatic; }
    glm::mat4 GetRootTransform() const { return m_RootTransform; }

private:
    std::vector<Texture> textures_loaded;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    bool m_ReadyToRender = false;
    bool m_IsStatic = false;
    glm::mat4 m_RootTransform = glm::mat4(1.0f);

    void loadModel(std::string const &path, bool isStatic);
    void ComputeAABB();
};

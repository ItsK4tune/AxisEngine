#pragma once

#include <core/unit/aabb.h>
#include <glm/glm.hpp>
#include <memory>
#include <render/logic/animdata.h>
#include <render/logic/mesh.h>
#include <render/logic/shader.h>
#include <string>
#include <unordered_map>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL

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

    void Draw(Shader &shader, bool bindTextures = true);
    void DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models, bool bindTextures = true);

    std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap();
    int &GetBoneCount();

    void AddTexture(const Texture& tex) { textures_loaded.push_back(tex); }

    std::string GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

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
    std::string m_Name;

    void loadModel(std::string const &path, bool isStatic);
    void ComputeAABB();
};
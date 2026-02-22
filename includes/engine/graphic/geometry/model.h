#pragma once

#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include <graphic/geometry/mesh.h>
#include <graphic/core/shader.h>
#include <graphic/geometry/animdata.h>

class Model
{
public:
    Model() = default;
    Model(std::string const &path, bool isStatic = false, bool gamma = false);
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection = false;
    glm::vec3 AABBmin = glm::vec3(0.0f);
    glm::vec3 AABBmax = glm::vec3(0.0f);

    void Draw(Shader &shader);
    void DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models);

    std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap();
    int &GetBoneCount();

    void AddTexture(const Texture& tex) { textures_loaded.push_back(tex); }

    void LoadCPU(std::string const &path, bool isStatic = false, bool gamma = false);

    bool IsReadyToRender() const { return m_ReadyToRender; }

private:
    std::vector<Texture> textures_loaded;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    bool m_ReadyToRender = false;

    void loadModel(std::string const &path, bool isStatic);
    void ComputeAABB();
};

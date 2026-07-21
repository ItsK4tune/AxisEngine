#pragma once

#include <core/unit/aabb.h>
#include <resource/unit/bone_info.h>
#include <resource/unit/mesh.h>
#include <resource/unit/shader.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>


class Model
{
public:
    Model() = default;
    Model(std::string const& path, bool isStatic = false, bool gamma = false);
    ~Model();
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    void LoadCPU(std::string const& path, bool isStatic = false, bool gamma = false);

    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection = false;
    AABB aabb;

    void Draw(Shader& shader, bool bindTextures = true);
    void DrawInstanced(Shader& shader, const std::vector<MeshInstanceData>& instances, bool bindTextures = true);

    std::unordered_map<std::string, BoneInfo>& GetBoneInfoMap();
    int& GetBoneCount();

    void AddTexture(const Texture& tex)
    {
        textures_loaded.push_back(tex);
    }

    std::string GetName() const
    {
        return m_Name;
    }
    void SetName(const std::string& name)
    {
        m_Name = name;
    }

    void UploadToGPU();
    // Transfers a fully decoded CPU model into this published handle. This is
    // intentionally called on the resource/main thread so async workers never
    // mutate a Model that the renderer can already observe.
    void AdoptCpuData(Model&& decoded);
    void ReleaseCpuMeshData();
    bool IsReadyToRender() const
    {
        return m_ReadyToRender;
    }
    bool IsStatic() const
    {
        return m_IsStatic;
    }
    void SetStatic(bool isStatic)
    {
        m_IsStatic = isStatic;
    }
    glm::mat4 GetRootTransform() const
    {
        return m_RootTransform;
    }
    glm::vec3 GetRootTranslation() const
    {
        return m_RootTranslation;
    }
    glm::quat GetRootRotation() const
    {
        return m_RootRotation;
    }
    glm::vec3 GetRootScale() const
    {
        return m_RootScale;
    }

private:
    std::vector<Texture> textures_loaded;
    std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
    int m_BoneCounter = 0;
    bool m_ReadyToRender = false;
    bool m_IsStatic = false;
    glm::mat4 m_RootTransform = glm::mat4(1.0f);
    glm::vec3 m_RootTranslation = glm::vec3(0.0f);
    glm::quat m_RootRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 m_RootScale = glm::vec3(1.0f);
    std::string m_Name;

    void loadModel(std::string const& path, bool isStatic);
    void ComputeAABB();
};

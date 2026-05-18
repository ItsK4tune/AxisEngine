#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <algorithm>
#include <cstring>
#include <fstream>

#define GLM_ENABLE_EXPERIMENTAL
#include <resource/unit/model.h>
#include <core/logic/filesystem.h>
#include <core/logic/logger.h>
#include <render/interface/i_texture_manager.h>
#include <render/logic/assimp_glm_helpers.h>
#include <glm/gtc/matrix_transform.hpp>
#include <stb/stb_image.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace
{

void SetVertexBoneDataToDefault(SkinnedVertex& Vertex)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        Vertex.m_BoneIDs[i] = -1;
        Vertex.m_Weights[i] = 0.0f;
    }
}

void SetVertexBoneData(SkinnedVertex& Vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (Vertex.m_BoneIDs[i] < 0)
        {
            Vertex.m_Weights[i] = weight;
            Vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

unsigned int TextureFromFile(const char* path, const std::string& directory, const aiScene* scene, bool deferred,
                             Texture* outTex = nullptr)
{
    std::string filename = std::string(path);
    std::replace(filename.begin(), filename.end(), '\\', '/');

    std::string directory_sanitized = directory;
    std::replace(directory_sanitized.begin(), directory_sanitized.end(), '\\', '/');

    std::string pureFilename = filename;
    size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != std::string::npos)
        pureFilename = filename.substr(lastSlash + 1);

    unsigned int textureID = 0;
    int width, height, nrComponents;
    unsigned char* data = nullptr;
    bool shouldFree = true;
    const aiTexture* aiTex = nullptr;

    if (filename[0] == '*')
    {
        try
        {
            int id = std::stoi(filename.substr(1));
            if (id < scene->mNumTextures)
                aiTex = scene->mTextures[id];
        }
        catch (...)
        {
        }
    }
    if (!aiTex)
        aiTex = scene->GetEmbeddedTexture(path);
    if (!aiTex)
        aiTex = scene->GetEmbeddedTexture(pureFilename.c_str());

    const int req_comp = 4;
    if (aiTex)
    {
        if (aiTex->mHeight == 0)
        {
            data = stbi_load_from_memory(reinterpret_cast<unsigned char*>(aiTex->pcData), aiTex->mWidth, &width,
                                         &height, &nrComponents, req_comp);
            shouldFree = true;
        }
        else
        {
            data = (unsigned char*)malloc(aiTex->mWidth * aiTex->mHeight * 4);
            memcpy(data, aiTex->pcData, aiTex->mWidth * aiTex->mHeight * 4);
            width = aiTex->mWidth;
            height = aiTex->mHeight;
            nrComponents = 4;
            shouldFree = true;
        }
    }
    else
    {
        std::string fullPath = directory_sanitized + '/' + pureFilename;
        data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, req_comp);

        if (!data)
        {
            std::string globalPath = FileSystem::getPath("include/engine/asset/textures") + '/' + pureFilename;
            data = stbi_load(globalPath.c_str(), &width, &height, &nrComponents, req_comp);
        }

        if (!data && filename != pureFilename)
        {
            std::string fullPathOriginal = directory_sanitized + '/' + filename;
            data = stbi_load(fullPathOriginal.c_str(), &width, &height, &nrComponents, req_comp);
        }
        shouldFree = true;
    }

    if (data)
    {
        if (deferred && outTex)
        {
            outTex->pixelData = data;
            outTex->width = width;
            outTex->height = height;
            outTex->nrComponents = nrComponents;
            return 0;
        }

        auto& tm = Mesh::GetTextureManager();
        textureID = tm.GenTexture();
        tm.BindTexture(TextureType::Texture2D, textureID);
        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, width, height, 0, TextureFormat::RGBA,
                      DataType::UnsignedByte, data);
        tm.GenerateMipmap(TextureType::Texture2D);
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::Repeat));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::Repeat));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                         static_cast<int>(TextureFilter::LinearMipmapLinear));
        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter, static_cast<int>(TextureFilter::Linear));
        if (shouldFree)
            stbi_image_free(data);
    }
    else
    {
        std::string fullPath = directory_sanitized + '/' + pureFilename;
        LOGGER_WARN("Model") << "Texture failed to load: " << pureFilename << " (Tried: " << fullPath << ")";
        return 0;
    }
    return textureID;
}

std::vector<Texture> loadMaterialTextures(std::vector<Texture>& textures_loaded, aiMaterial* mat, aiTextureType type,
                                          std::string typeName, const std::string& directory, const aiScene* scene,
                                          bool deferred)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);

        std::string rawPath = str.C_Str();
        std::replace(rawPath.begin(), rawPath.end(), '\\', '/');
        std::string filename = rawPath;
        size_t lastSlash = rawPath.find_last_of('/');
        if (lastSlash != std::string::npos)
            filename = rawPath.substr(lastSlash + 1);

        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            std::string loadedName = textures_loaded[j].path;
            std::replace(loadedName.begin(), loadedName.end(), '\\', '/');
            size_t ls = loadedName.find_last_of('/');
            if (ls != std::string::npos)
                loadedName = loadedName.substr(ls + 1);
            if (filename == loadedName)
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip)
        {
            Texture Texture;
            Texture.type = typeName;
            Texture.path = str.C_Str();

            unsigned int id = TextureFromFile(str.C_Str(), directory, scene, deferred, &Texture);

            if (id != 0 || (deferred && Texture.pixelData != nullptr))
            {
                Texture.id = id;
                textures.push_back(Texture);
                textures_loaded.push_back(Texture);
            }
        }
    }
    return textures;
}

void ExtractBoneWeightForVertices(std::vector<SkinnedVertex>& vertices, aiMesh* mesh,
                                  std::unordered_map<std::string, BoneInfo>& boneInfoMap, int& boneCount)
{
    for (int boneIndex = 0; boneIndex < (int)mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (boneInfoMap.find(boneName) == boneInfoMap.end())
        {
            if (boneCount < 200)
            {
                BoneInfo newBoneInfo;
                newBoneInfo.id = boneCount;
                newBoneInfo.offset = AssimpGLMHelpers::ConvertMatrixToGLMFormat(mesh->mBones[boneIndex]->mOffsetMatrix);
                boneInfoMap[boneName] = newBoneInfo;
                boneID = boneCount;
                boneCount++;
            }
            else
            {
                LOGGER_WARN("Model") << "Max bone limit (200) exceeded while processing bone: " << boneName;
                continue;
            }
        }
        else
        {
            boneID = boneInfoMap[boneName].id;
        }
        assert(boneID != -1);
        auto weights = mesh->mBones[boneIndex]->mWeights;
        int numWeights = mesh->mBones[boneIndex]->mNumWeights;
        for (int weightIndex = 0; weightIndex < numWeights; ++weightIndex)
        {
            int vertexId = weights[weightIndex].mVertexId;
            float weight = weights[weightIndex].mWeight;
            if (vertexId < (int)vertices.size())
                SetVertexBoneData(vertices[vertexId], boneID, weight);
        }
    }

    if (mesh->mNumBones > 0)
    {
        LOGGER_INFO("Model") << "Mesh " << mesh->mName.C_Str() << " has " << mesh->mNumBones
                             << " bones (Total unique bones in model: " << boneCount << ")";
    }
}

Mesh processMesh(aiMesh* mesh, const aiScene* scene, std::vector<Texture>& textures_loaded,
                 const std::string& directory, std::unordered_map<std::string, BoneInfo>& boneInfoMap, int& boneCount,
                 bool deferred)
{
    bool hasBones = mesh->HasBones();
    std::vector<uint8_t> vertexData;
    size_t vertexStride = hasBones ? sizeof(SkinnedVertex) : sizeof(StaticVertex);
    vertexData.resize(mesh->mNumVertices * vertexStride);

    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    indices.reserve(mesh->mNumFaces * 3);

    if (hasBones)
    {
        SkinnedVertex* skinnedData = reinterpret_cast<SkinnedVertex*>(vertexData.data());
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            SkinnedVertex& v = skinnedData[i];
            SetVertexBoneDataToDefault(v);
            v.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            v.Normal = mesh->HasNormals() ? AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]) : glm::vec3(0);
            v.TexCoords = mesh->mTextureCoords[0]
                              ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                              : glm::vec2(0);
        }
        // Temporarily wrap for bone extraction
        std::vector<SkinnedVertex> wrap(skinnedData, skinnedData + mesh->mNumVertices);
        ExtractBoneWeightForVertices(wrap, mesh, boneInfoMap, boneCount);
        // Copy back
        std::copy(wrap.begin(), wrap.end(), skinnedData);
    }
    else
    {
        StaticVertex* staticData = reinterpret_cast<StaticVertex*>(vertexData.data());
        for (unsigned int i = 0; i < mesh->mNumVertices; i++)
        {
            StaticVertex& v = staticData[i];
            v.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
            v.Normal = mesh->HasNormals() ? AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]) : glm::vec3(0);
            v.TexCoords = mesh->mTextureCoords[0]
                              ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                              : glm::vec2(0);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> diffuseMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_BASE_COLOR,
                                                                "texture_diffuse", directory, scene, deferred);
        if (diffuseMaps.empty())
        {
            diffuseMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_DIFFUSE, "texture_diffuse",
                                               directory, scene, deferred);
            if (diffuseMaps.empty())
            {
                aiColor4D color;
                if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS ||
                    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
                {
                    unsigned char* data = (unsigned char*)malloc(4);
                    data[0] = (unsigned char)(color.r * 255.0f);
                    data[1] = (unsigned char)(color.g * 255.0f);
                    data[2] = (unsigned char)(color.b * 255.0f);
                    data[3] = (unsigned char)(color.a * 255.0f);

                    Texture Texture;
                    Texture.pixelData = data;
                    Texture.width = 1;
                    Texture.height = 1;
                    Texture.nrComponents = 4;
                    Texture.type = "texture_diffuse";
                    Texture.path = "INTERNAL_COLOR_FALLBACK_" + std::to_string(mesh->mMaterialIndex);

                    if (!deferred)
                    {
                        auto& tm = Mesh::GetTextureManager();
                        Texture.id = tm.GenTexture();
                        tm.BindTexture(TextureType::Texture2D, Texture.id);
                        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA,
                                      DataType::UnsignedByte, data);
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS,
                                         static_cast<int>(TextureWrap::Repeat));
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT,
                                         static_cast<int>(TextureWrap::Repeat));
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                         static_cast<int>(TextureFilter::Nearest));
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                         static_cast<int>(TextureFilter::Nearest));
                        free(data);
                        Texture.pixelData = nullptr;
                    }
                    diffuseMaps.push_back(Texture);
                }
                else
                {
                    unsigned char* data = (unsigned char*)malloc(4);
                    data[0] = 255;
                    data[1] = 0;
                    data[2] = 255;
                    data[3] = 255;

                    Texture Texture;
                    Texture.pixelData = data;
                    Texture.width = 1;
                    Texture.height = 1;
                    Texture.nrComponents = 4;
                    Texture.type = "texture_diffuse";
                    Texture.path = "INTERNAL_MAGENTA_FALLBACK";

                    if (!deferred)
                    {
                        auto& tm = Mesh::GetTextureManager();
                        Texture.id = tm.GenTexture();
                        tm.BindTexture(TextureType::Texture2D, Texture.id);
                        tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA,
                                      DataType::UnsignedByte, data);
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                                         static_cast<int>(TextureFilter::Nearest));
                        tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                                         static_cast<int>(TextureFilter::Nearest));
                        free(data);
                        Texture.pixelData = nullptr;
                    }
                    diffuseMaps.push_back(Texture);
                }
            }
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_SPECULAR,
                                                                 "texture_specular", directory, scene, deferred);
        if (specularMaps.empty())
        {
            unsigned char data[4] = {255, 0, 255, 255};
            auto& tm = Mesh::GetTextureManager();
            unsigned int textureID = tm.GenTexture();
            tm.BindTexture(TextureType::Texture2D, textureID);
            tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, 1, 1, 0, TextureFormat::RGBA,
                          DataType::UnsignedByte, data);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                             static_cast<int>(TextureFilter::Nearest));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                             static_cast<int>(TextureFilter::Nearest));
            Texture Texture;
            Texture.id = textureID;
            Texture.type = "texture_specular";
            Texture.path = "INTERNAL_SPECULAR_FALLBACK";
            specularMaps.push_back(Texture);
        }
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_NORMALS,
                                                               "texture_normal", directory, scene, deferred);
        if (normalMaps.empty())
            normalMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_HEIGHT, "texture_normal",
                                              directory, scene, deferred);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> metallicMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_METALNESS,
                                                                 "texture_metallic", directory, scene, deferred);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        std::vector<Texture> roughnessMaps =
            loadMaterialTextures(textures_loaded, material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness",
                                 directory, scene, deferred);
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

        std::vector<Texture> aoMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_AMBIENT_OCCLUSION,
                                                           "texture_ao", directory, scene, deferred);
        if (aoMaps.empty())
            aoMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_LIGHTMAP, "texture_ao", directory,
                                          scene, deferred);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    }

    return Mesh(std::move(vertexData), mesh->mNumVertices, vertexStride, hasBones, std::move(indices),
                std::move(textures), !deferred);
}

void processNode(aiNode* node, const aiScene* scene, std::vector<Mesh>& meshes, std::vector<Texture>& textures_loaded,
                 const std::string& directory, std::unordered_map<std::string, BoneInfo>& boneInfoMap, int& boneCount,
                 bool deferred)
{
    if (!node || !scene || !scene->mMeshes)
        return;
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex >= scene->mNumMeshes)
            continue;
        aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh)
            meshes.push_back(processMesh(mesh, scene, textures_loaded, directory, boneInfoMap, boneCount, deferred));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene, meshes, textures_loaded, directory, boneInfoMap, boneCount, deferred);
}

}  // namespace

Model::Model(std::string const& path, bool isStatic, bool gamma) : gammaCorrection(gamma), m_IsStatic(isStatic)
{
    loadModel(path, isStatic);
    ComputeAABB();
}

Model::~Model()
{
    for (auto& tex : textures_loaded)
    {
        if (tex.pixelData)
        {
            stbi_image_free(tex.pixelData);
            tex.pixelData = nullptr;
        }
    }
}

void Model::LoadCPU(std::string const& path, bool isStatic, bool gamma)
{
    gammaCorrection = gamma;
    m_IsStatic = isStatic;
    loadModel(path, isStatic);
    ComputeAABB();
}

void Model::ComputeAABB()
{
    if (!meshes.empty())
    {
        glm::mat4 root = GetRootTransform();
        aabb.minBound = glm::vec3(root * glm::vec4(meshes[0].aabb.minBound, 1.0f));
        aabb.maxBound = glm::vec3(root * glm::vec4(meshes[0].aabb.maxBound, 1.0f));

        for (const auto& mesh : meshes)
        {
            glm::vec3 worldMin = glm::vec3(root * glm::vec4(mesh.aabb.minBound, 1.0f));
            glm::vec3 worldMax = glm::vec3(root * glm::vec4(mesh.aabb.maxBound, 1.0f));

            aabb.minBound.x = (std::min)(aabb.minBound.x, worldMin.x);
            aabb.minBound.y = (std::min)(aabb.minBound.y, worldMin.y);
            aabb.minBound.z = (std::min)(aabb.minBound.z, worldMin.z);

            aabb.maxBound.x = (std::max)(aabb.maxBound.x, worldMax.x);
            aabb.maxBound.y = (std::max)(aabb.maxBound.y, worldMax.y);
            aabb.maxBound.z = (std::max)(aabb.maxBound.z, worldMax.z);
        }
    }
}

void Model::Draw(Shader& shader, bool bindTextures)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
    {
        meshes[i].Draw(shader, bindTextures);
    }
}

void Model::DrawInstanced(Shader& shader, const std::vector<glm::mat4>& models, bool bindTextures)
{
    for (unsigned int i = 0; i < meshes.size(); i++) meshes[i].DrawInstanced(shader, models, bindTextures);
}

std::unordered_map<std::string, BoneInfo>& Model::GetBoneInfoMap()
{
    return m_BoneInfoMap;
}
int& Model::GetBoneCount()
{
    return m_BoneCounter;
}

void Model::loadModel(std::string const& path, bool isStatic)
{
    m_IsStatic = isStatic;
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs;

    if (m_IsStatic)
        flags |= aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(path, flags);
    if (scene && scene->mRootNode)
    {
        m_RootTransform = AssimpGLMHelpers::ConvertMatrixToGLMFormat(scene->mRootNode->mTransformation);
        m_RootTransform = glm::scale(m_RootTransform, glm::vec3(0.01f));
    }

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        LOGGER_WARN("Model") << "ERROR::ASSIMP:: " << importer.GetErrorString();
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));

    bool deferred = !m_ReadyToRender;

    processNode(scene->mRootNode, scene, meshes, textures_loaded, directory, m_BoneInfoMap, m_BoneCounter, deferred);
}

void Model::UploadToGPU()
{
    if (m_ReadyToRender)
        return;

    auto& tm = Mesh::GetTextureManager();

    for (auto& tex : textures_loaded)
    {
        if (tex.id == 0 && tex.pixelData)
        {
            tex.id = tm.GenTexture();
            tm.BindTexture(TextureType::Texture2D, tex.id);
            tm.TexImage2D(TextureType::Texture2D, 0, InternalFormat::RGBA8, tex.width, tex.height, 0,
                          TextureFormat::RGBA, DataType::UnsignedByte, tex.pixelData);
            tm.GenerateMipmap(TextureType::Texture2D);
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapS, static_cast<int>(TextureWrap::Repeat));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::WrapT, static_cast<int>(TextureWrap::Repeat));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MinFilter,
                             static_cast<int>(TextureFilter::LinearMipmapLinear));
            tm.TexParameteri(TextureType::Texture2D, TextureParameter::MagFilter,
                             static_cast<int>(TextureFilter::Linear));

            stbi_image_free(tex.pixelData);
            tex.pixelData = nullptr;
        }
    }

    for (auto& mesh : meshes)
    {
        for (auto& meshTex : mesh.textures)
        {
            if (meshTex.id == 0)
            {
                for (const auto& loadedTex : textures_loaded)
                {
                    if (loadedTex.path == meshTex.path)
                    {
                        meshTex.id = loadedTex.id;
                        break;
                    }
                }
            }
        }
        mesh.setupMesh();
    }

    m_ReadyToRender = true;
}

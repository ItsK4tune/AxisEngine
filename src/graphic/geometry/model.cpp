#include <graphic/geometry/model.h>

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>

#include <utils/assimp_glm_helpers.h>
#include <interface/graphic/i_texture_manager.h>

namespace {

void SetVertexBoneDataToDefault(Vertex& vertex)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.0f;
    }
}

void SetVertexBoneData(Vertex& vertex, int boneID, float weight)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
    {
        if (vertex.m_BoneIDs[i] < 0)
        {
            vertex.m_Weights[i] = weight;
            vertex.m_BoneIDs[i] = boneID;
            break;
        }
    }
}

unsigned int TextureFromFile(const char* path, const std::string& directory, const aiScene* scene, bool gamma = false)
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
        catch (...) {}
    }
    if (!aiTex) aiTex = scene->GetEmbeddedTexture(path);
    if (!aiTex) aiTex = scene->GetEmbeddedTexture(pureFilename.c_str());

    const int req_comp = 4;
    if (aiTex)
    {
        if (aiTex->mHeight == 0)
        {
            data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(aiTex->pcData),
                aiTex->mWidth, &width, &height, &nrComponents, req_comp);
            shouldFree = true;
        }
        else
        {
            data = reinterpret_cast<unsigned char*>(aiTex->pcData);
            width = aiTex->mWidth;
            height = aiTex->mHeight;
            nrComponents = 4;
            shouldFree = false;
        }
    }
    else
    {
        std::string fullPath = directory_sanitized + '/' + pureFilename;
        data = stbi_load(fullPath.c_str(), &width, &height, &nrComponents, req_comp);
        if (!data && filename != pureFilename)
        {
            std::string fullPathOriginal = directory_sanitized + '/' + filename;
            data = stbi_load(fullPathOriginal.c_str(), &width, &height, &nrComponents, req_comp);
        }
        shouldFree = true;
    }

    if (data)
    {
        auto& tm = Mesh::GetTextureManager();
        textureID = tm.GenTexture();
        tm.BindTexture(Graphics::TextureType::Texture2D, textureID);
        tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, width, height, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, data);
        tm.GenerateMipmap(Graphics::TextureType::Texture2D);
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::Repeat));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::Repeat));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::LinearMipmapLinear));
        tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Linear));
        if (shouldFree) stbi_image_free(data);
    }
    else
    {
        std::string fullPath = directory_sanitized + '/' + pureFilename;
        std::cout << "[Model] Texture failed to load: " << pureFilename
                  << " (Tried: " << fullPath << ")" << std::endl;
        return 0;
    }
    return textureID;
}

std::vector<Texture> loadMaterialTextures(std::vector<Texture>& textures_loaded,
                                           aiMaterial* mat, aiTextureType type,
                                           std::string typeName, const std::string& directory,
                                           const aiScene* scene)
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
            unsigned int id = TextureFromFile(str.C_Str(), directory, scene);
            if (id != 0)
            {
                Texture texture;
                texture.id = id;
                texture.type = typeName;
                texture.path = str.C_Str();
                textures.push_back(texture);
                textures_loaded.push_back(texture);
            }
        }
    }
    return textures;
}

void ExtractBoneWeightForVertices(std::vector<Vertex>& vertices, aiMesh* mesh,
                                   std::unordered_map<std::string, BoneInfo>& boneInfoMap,
                                   int& boneCount)
{
    for (int boneIndex = 0; boneIndex < (int)mesh->mNumBones; ++boneIndex)
    {
        int boneID = -1;
        std::string boneName = mesh->mBones[boneIndex]->mName.C_Str();
        if (boneInfoMap.find(boneName) == boneInfoMap.end())
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
}

Mesh processMesh(aiMesh* mesh, const aiScene* scene,
                  std::vector<Texture>& textures_loaded, const std::string& directory,
                  std::unordered_map<std::string, BoneInfo>& boneInfoMap, int& boneCount)
{
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces * 3);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++)
    {
        Vertex vertex;
        SetVertexBoneDataToDefault(vertex);
        vertex.Position = AssimpGLMHelpers::GetGLMVec(mesh->mVertices[i]);
        vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
        if (mesh->HasNormals())
            vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            vertex.Tangent = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
            vertex.Bitangent = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);
        }
        else
        {
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        }
        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0)
    {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<Texture> diffuseMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_BASE_COLOR, "texture_diffuse", directory, scene);
        if (diffuseMaps.empty())
        {
            diffuseMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_DIFFUSE, "texture_diffuse", directory, scene);
            if (diffuseMaps.empty())
            {
                aiColor4D color;
                if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS ||
                    aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
                {
                    unsigned char data[4];
                    data[0] = (unsigned char)(color.r * 255.0f);
                    data[1] = (unsigned char)(color.g * 255.0f);
                    data[2] = (unsigned char)(color.b * 255.0f);
                    data[3] = (unsigned char)(color.a * 255.0f);
                    auto& tm = Mesh::GetTextureManager();
                    unsigned int textureID = tm.GenTexture();
                    tm.BindTexture(Graphics::TextureType::Texture2D, textureID);
                    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, 1, 1, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, data);
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapS, static_cast<int>(Graphics::TextureWrap::Repeat));
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::WrapT, static_cast<int>(Graphics::TextureWrap::Repeat));
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
                    Texture texture;
                    texture.id = textureID;
                    texture.type = "texture_diffuse";
                    texture.path = "INTERNAL_COLOR_FALLBACK";
                    diffuseMaps.push_back(texture);
                }
                else
                {
                    unsigned char data[4] = { 255, 255, 255, 255 };
                    auto& tm = Mesh::GetTextureManager();
                    unsigned int textureID = tm.GenTexture();
                    tm.BindTexture(Graphics::TextureType::Texture2D, textureID);
                    tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, 1, 1, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, data);
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
                    tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
                    Texture texture;
                    texture.id = textureID;
                    texture.type = "texture_diffuse";
                    texture.path = "INTERNAL_WHITE_FALLBACK";
                    diffuseMaps.push_back(texture);
                }
            }
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<Texture> specularMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_SPECULAR, "texture_specular", directory, scene);
        if (specularMaps.empty())
        {
            unsigned char data[4] = {255, 255, 255, 255};
            auto& tm = Mesh::GetTextureManager();
            unsigned int textureID = tm.GenTexture();
            tm.BindTexture(Graphics::TextureType::Texture2D, textureID);
            tm.TexImage2D(Graphics::TextureType::Texture2D, 0, Graphics::InternalFormat::RGBA8, 1, 1, 0, Graphics::TextureFormat::RGBA, Graphics::DataType::UnsignedByte, data);
            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MinFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
            tm.TexParameteri(Graphics::TextureType::Texture2D, Graphics::TextureParameter::MagFilter, static_cast<int>(Graphics::TextureFilter::Nearest));
            Texture texture;
            texture.id = textureID;
            texture.type = "texture_specular";
            texture.path = "INTERNAL_SPECULAR_FALLBACK";
            specularMaps.push_back(texture);
        }
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        std::vector<Texture> normalMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_NORMALS, "texture_normal", directory, scene);
        if (normalMaps.empty())
            normalMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_HEIGHT, "texture_normal", directory, scene);
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        std::vector<Texture> metallicMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_METALNESS, "texture_metallic", directory, scene);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        std::vector<Texture> roughnessMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", directory, scene);
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

        std::vector<Texture> aoMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", directory, scene);
        if (aoMaps.empty())
            aoMaps = loadMaterialTextures(textures_loaded, material, aiTextureType_LIGHTMAP, "texture_ao", directory, scene);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    }

    ExtractBoneWeightForVertices(vertices, mesh, boneInfoMap, boneCount);
    return Mesh(vertices, indices, textures);
}

void processNode(aiNode* node, const aiScene* scene,
                  std::vector<Mesh>& meshes, std::vector<Texture>& textures_loaded,
                  const std::string& directory,
                  std::unordered_map<std::string, BoneInfo>& boneInfoMap, int& boneCount)
{
    if (!node || !scene || !scene->mMeshes) return;
    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex >= scene->mNumMeshes) continue;
        aiMesh* mesh = scene->mMeshes[meshIndex];
        if (mesh)
            meshes.push_back(processMesh(mesh, scene, textures_loaded, directory, boneInfoMap, boneCount));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++)
        processNode(node->mChildren[i], scene, meshes, textures_loaded, directory, boneInfoMap, boneCount);
}

}

Model::Model(std::string const &path, bool isStatic, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path, isStatic);
    ComputeAABB();
    m_ReadyToRender = true;
}

void Model::LoadCPU(std::string const &path, bool isStatic, bool gamma)
{
    gammaCorrection = gamma;
    loadModel(path, isStatic);
    ComputeAABB();
    m_ReadyToRender = true;
}

void Model::ComputeAABB()
{
    if (!meshes.empty())
    {
        AABBmin = meshes[0].AABBmin;
        AABBmax = meshes[0].AABBmax;
        for (const auto &mesh : meshes)
        {
            AABBmin.x = (std::min)(AABBmin.x, mesh.AABBmin.x);
            AABBmin.y = (std::min)(AABBmin.y, mesh.AABBmin.y);
            AABBmin.z = (std::min)(AABBmin.z, mesh.AABBmin.z);

            AABBmax.x = (std::max)(AABBmax.x, mesh.AABBmax.x);
            AABBmax.y = (std::max)(AABBmax.y, mesh.AABBmax.y);
            AABBmax.z = (std::max)(AABBmax.z, mesh.AABBmax.z);
        }
    }
}

void Model::Draw(Shader &shader)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].Draw(shader);
}

void Model::DrawInstanced(Shader &shader, const std::vector<glm::mat4> &models)
{
    for (unsigned int i = 0; i < meshes.size(); i++)
        meshes[i].DrawInstanced(shader, models);
}

std::unordered_map<std::string, BoneInfo> &Model::GetBoneInfoMap() { return m_BoneInfoMap; }
int &Model::GetBoneCount() { return m_BoneCounter; }

void Model::loadModel(std::string const &path, bool isStatic)
{
    Assimp::Importer importer;

    unsigned int flags = aiProcess_Triangulate |
                         aiProcess_GenSmoothNormals |
                         aiProcess_CalcTangentSpace |
                         aiProcess_FlipUVs;

    if (isStatic)
        flags |= aiProcess_PreTransformVertices;

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "[Model] ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene, meshes, textures_loaded, directory, m_BoneInfoMap, m_BoneCounter);
}

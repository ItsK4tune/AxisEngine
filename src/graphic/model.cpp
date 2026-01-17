#include <graphic/model.h>

#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm> // Cho std::max nếu cần

#include <utils/assimp_glm_helpers.h>

Model::Model(std::string const &path, bool isStatic, bool gamma) : gammaCorrection(gamma)
{
    loadModel(path, isStatic);

    // Calculate Model AABB from meshes
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

    if (isStatic) {
        flags |= aiProcess_PreTransformVertices;
    }
    
    const aiScene *scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "[Model] ERROR::ASSIMP:: " << importer.GetErrorString() << std::endl;
        return;
    }

    directory = path.substr(0, path.find_last_of('/'));
    processNode(scene->mRootNode, scene);
}

void Model::processNode(aiNode *node, const aiScene *scene)
{
    if (!node || !scene || !scene->mMeshes)
        return;

    for (unsigned int i = 0; i < node->mNumMeshes; i++)
    {
        unsigned int meshIndex = node->mMeshes[i];
        if (meshIndex >= scene->mNumMeshes) continue;

        aiMesh *mesh = scene->mMeshes[meshIndex];
        if (mesh) {
            meshes.push_back(processMesh(mesh, scene));
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

void Model::SetVertexBoneDataToDefault(Vertex &vertex)
{
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++)
    {
        vertex.m_BoneIDs[i] = -1;
        vertex.m_Weights[i] = 0.0f;
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene)
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

        if (mesh->HasNormals()) {
            vertex.Normal = AssimpGLMHelpers::GetGLMVec(mesh->mNormals[i]);
        }

        if (mesh->mTextureCoords[0]) {
            glm::vec2 vec;
            vec.x = mesh->mTextureCoords[0][i].x;
            vec.y = mesh->mTextureCoords[0][i].y;
            vertex.TexCoords = vec;
            vertex.Tangent = AssimpGLMHelpers::GetGLMVec(mesh->mTangents[i]);
            vertex.Bitangent = AssimpGLMHelpers::GetGLMVec(mesh->mBitangents[i]);
        }
        else {
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
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        // 1. Diffuse / Albedo
        // PBR: BASE_COLOR
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse", scene);
        if (diffuseMaps.empty()) {
            // Phong: DIFFUSE
            diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse", scene);
        }
        // Fallback color
        if (diffuseMaps.empty())
        {
             aiColor4D color;
             if (aiGetMaterialColor(material, AI_MATKEY_BASE_COLOR, &color) == AI_SUCCESS || 
                 aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
             {
                 // Create 1x1 color texture ... (Existing logic)
                 unsigned char data[4];
                 data[0] = (unsigned char)(color.r * 255.0f);
                 data[1] = (unsigned char)(color.g * 255.0f);
                 data[2] = (unsigned char)(color.b * 255.0f);
                 data[3] = (unsigned char)(color.a * 255.0f);
                 unsigned int textureID;
                 glGenTextures(1, &textureID);
                 glBindTexture(GL_TEXTURE_2D, textureID);
                 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                 glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                 Texture texture;
                 texture.id = textureID;
                 texture.type = "texture_diffuse";
                 texture.path = "INTERNAL_COLOR_FALLBACK";
                 diffuseMaps.push_back(texture);
             }
        }
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        // 2. Specular (Phong)
        std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular", scene);
        if (specularMaps.empty()) {
             // Default white specular for Phong if missing
            unsigned char data[4] = {255, 255, 255, 255};
            unsigned int textureID;
            glGenTextures(1, &textureID);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            Texture texture;
            texture.id = textureID;
            texture.type = "texture_specular"; 
            texture.path = "INTERNAL_SPECULAR_FALLBACK";
            specularMaps.push_back(texture);
        }
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());

        // 3. Normal
        std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_NORMALS, "texture_normal", scene);
        if (normalMaps.empty()) {
             normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal", scene);
        }
        textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());

        // 4. PBR Maps
        // Metallic
        std::vector<Texture> metallicMaps = loadMaterialTextures(material, aiTextureType_METALNESS, "texture_metallic", scene);
        textures.insert(textures.end(), metallicMaps.begin(), metallicMaps.end());

        // Roughness
        std::vector<Texture> roughnessMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE_ROUGHNESS, "texture_roughness", scene);
        textures.insert(textures.end(), roughnessMaps.begin(), roughnessMaps.end());

        // AO
        std::vector<Texture> aoMaps = loadMaterialTextures(material, aiTextureType_AMBIENT_OCCLUSION, "texture_ao", scene);
        // Sometimes AO is in Lightmap
        if (aoMaps.empty()) aoMaps = loadMaterialTextures(material, aiTextureType_LIGHTMAP, "texture_ao", scene);
        textures.insert(textures.end(), aoMaps.begin(), aoMaps.end());
    }

    ExtractBoneWeightForVertices(vertices, mesh, scene);

    return Mesh(vertices, indices, textures);
}

void Model::SetVertexBoneData(Vertex &vertex, int boneID, float weight)
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

void Model::ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene)
{
    auto &boneInfoMap = m_BoneInfoMap;
    int &boneCount = m_BoneCounter;

    for (int boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
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
            if (vertexId < vertices.size())
            {
                SetVertexBoneData(vertices[vertexId], boneID, weight);
            }
        }
    }
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory, const aiScene *scene, bool gamma)
{
    std::string filename = std::string(path);
    std::replace(filename.begin(), filename.end(), '\\', '/');

    std::string directory_sanitized = directory;
    std::replace(directory_sanitized.begin(), directory_sanitized.end(), '\\', '/');

    std::string pureFilename = filename;
    size_t lastSlash = filename.find_last_of('/');
    if (lastSlash != std::string::npos) {
        pureFilename = filename.substr(lastSlash + 1);
    }

    unsigned int textureID = 0;

    int width, height, nrComponents;
    unsigned char *data = nullptr;
    bool shouldFree = true;

    const aiTexture* aiTex = nullptr;

    if (filename[0] == '*') {
        try {
            int id = std::stoi(filename.substr(1));
            if(id < scene->mNumTextures) aiTex = scene->mTextures[id];
        } catch (...) {}
    }
    
    if (!aiTex) {
        aiTex = scene->GetEmbeddedTexture(path);
    }
    if (!aiTex) {
        aiTex = scene->GetEmbeddedTexture(pureFilename.c_str());
    }

    const int req_comp = 4; 

    if (aiTex) 
    {
        if (aiTex->mHeight == 0) {
            data = stbi_load_from_memory(
                reinterpret_cast<unsigned char*>(aiTex->pcData),
                aiTex->mWidth,
                &width, &height, &nrComponents, req_comp);
            shouldFree = true;
        } 
        else {
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
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        if (shouldFree) {
            stbi_image_free(data);
        }
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

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene *scene)
{
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++)
    {
        aiString str;
        mat->GetTexture(type, i, &str);
        
        std::string rawPath = str.C_Str();
        std::replace(rawPath.begin(), rawPath.end(), '\\', '/'); // Fix separator
        
        std::string filename = rawPath;
        size_t lastSlash = rawPath.find_last_of('/');
        if (lastSlash != std::string::npos) {
            filename = rawPath.substr(lastSlash + 1);
        }

        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++)
        {
            std::string loadedName = textures_loaded[j].path;
            
            // Fix loaded path for comparison
            std::replace(loadedName.begin(), loadedName.end(), '\\', '/');
            size_t ls = loadedName.find_last_of('/');
             if(ls != std::string::npos) loadedName = loadedName.substr(ls+1);

            if (filename == loadedName)
            {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        
        if (!skip)
        {
            unsigned int id = TextureFromFile(str.C_Str(), this->directory, scene);
            if (id != 0) // Only add if valid
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
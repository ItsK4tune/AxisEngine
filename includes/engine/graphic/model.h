#pragma once

#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <unordered_map>

#include <engine/graphic/mesh.h>
#include <engine/graphic/shader.h>
#include <engine/graphic/animdata.h>

class Model
{
public:
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;

	Model(std::string const &path, bool gamma = false);

	void Draw(Shader &shader);

	std::unordered_map<std::string, BoneInfo> &GetBoneInfoMap();
	int &GetBoneCount();

private:
	std::unordered_map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

	void loadModel(std::string const &path);
	void processNode(aiNode *node, const aiScene *scene);
	void SetVertexBoneDataToDefault(Vertex &vertex);
	Mesh processMesh(aiMesh *mesh, const aiScene *scene);
	void SetVertexBoneData(Vertex &vertex, int boneID, float weight);
	void ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene);
	
	unsigned int TextureFromFile(const char *path, const std::string &directory, const aiScene* scene, bool gamma = false);
	std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName, const aiScene* scene);
};
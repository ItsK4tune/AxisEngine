#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <assimp/scene.h>

#include <engine/mesh.h>
#include <engine/shader.h>

#include <map>

#include <engine/animdata.h>

class Model
{
public:
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> meshes;
	std::string directory;
	bool gammaCorrection;

	Model(std::string const &path, bool gamma = false);

	void Draw(Shader &shader);

	std::map<std::string, BoneInfo> &GetBoneInfoMap();
	int &GetBoneCount();

private:
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	int m_BoneCounter = 0;

	void loadModel(std::string const &path);
	void processNode(aiNode *node, const aiScene *scene);
	void SetVertexBoneDataToDefault(Vertex &vertex);
	Mesh processMesh(aiMesh *mesh, const aiScene *scene);
	void SetVertexBoneData(Vertex &vertex, int boneID, float weight);
	void ExtractBoneWeightForVertices(std::vector<Vertex> &vertices, aiMesh *mesh, const aiScene *scene);
	unsigned int TextureFromFile(const char *path, const std::string &directory, bool gamma = false);
	std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
};
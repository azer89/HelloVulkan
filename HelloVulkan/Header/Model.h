#ifndef MODEL
#define MODEL

#include "Mesh.h"
#include "TextureMapper.h"
#include "VulkanContext.h"
#include "VulkanImage.h"

#include <string>
#include <vector>
#include <unordered_map>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"

class Model
{
private:
	// Model data 
	std::unordered_map<std::string, VulkanImage> textureMap_; // key is the filename
	std::string directory_;
	std::string blackTextureFilePath_;
	VkDevice device_;

public:

	std::vector<Mesh> meshes_;

	// ModelUBO
	std::vector<VulkanBuffer> modelBuffers_;

public:
	// Constructor, expects a filepath to a 3D model.
	Model(VulkanContext& vkDev, const std::string& path);

	// Destructor
	~Model();

	void AddTextureIfEmpty(TextureType tType, const std::string& filePath);

	uint32_t NumMeshes()
	{
		return static_cast<uint32_t>(meshes_.size());
	}

	// TODO Probably move the buffers to pipelines
	void SetModelUBO(VulkanContext& vkDev, ModelUBO ubo)
	{
		uint32_t frameIndex = vkDev.GetFrameIndex();
		modelBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(ModelUBO));
	}

private:
	// Loads a model with supported ASSIMP extensions from file and 
	// stores the resulting meshes in the meshes vector.
	void LoadModel(
		VulkanContext& vkDev, 
		std::string const& path);

	// Processes a node in a recursive fashion. 
	void ProcessNode(
		VulkanContext& vkDev, 
		aiNode* node, 
		const aiScene* scene, 
		const glm::mat4& parentTransform);

	Mesh ProcessMesh(
		VulkanContext& vkDev, 
		aiMesh* mesh, 
		const aiScene* scene, 
		const glm::mat4& transform);
};

#endif


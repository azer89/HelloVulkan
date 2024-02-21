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
	VkDevice device_;
	std::string directory_;

	// PBR Textures
	std::vector<VulkanImage> textureList_;
	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_;

public:
	std::vector<Mesh> meshes_;

	// Bindless rendering
	std::vector<VertexData> vertices_;
	std::vector<uint32_t> indices_;
	VkDeviceSize vertexBufferSize_;
	VkDeviceSize indexBufferSize_;
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;

	// TODO Separate buffers from the model
	std::vector<VulkanBuffer> modelBuffers_;

public:
	// Constructor, expects a filepath to a 3D model.
	Model(VulkanContext& ctx, const std::string& path);

	// Destructor
	~Model();

	VulkanImage* GetTexture(uint32_t textureIndex);

	void AddTextureIfEmpty(TextureType tType, const std::string& filePath);

	uint32_t NumMeshes()
	{
		return static_cast<uint32_t>(meshes_.size());
	}

	// TODO Probably move the buffers to pipelines
	void SetModelUBO(VulkanContext& ctx, ModelUBO ubo)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		modelBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ModelUBO));
	}

private:
	void AddTexture(VulkanContext& ctx, const std::string& textureFilename);
	void AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height);

	// Loads a model with supported ASSIMP extensions from file and 
	// stores the resulting meshes in the meshes vector.
	void LoadModel(
		VulkanContext& ctx, 
		std::string const& path);

	// Processes a node recursively. 
	void ProcessNode(
		VulkanContext& ctx, 
		uint32_t& vertexOffset,
		uint32_t& indexOffset,
		aiNode* node, 
		const aiScene* scene, 
		const glm::mat4& parentTransform);

	Mesh ProcessMesh(
		VulkanContext& ctx, 
		uint32_t& vertexOffset,
		uint32_t& indexOffset,
		aiMesh* mesh, 
		const aiScene* scene, 
		const glm::mat4& transform);

	void CreateBuffers(VulkanContext& ctx);
};

#endif


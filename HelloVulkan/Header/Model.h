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
	bool bindless_;

	// PBR Textures
	std::vector<VulkanImage> textureList_;
	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_;

public:
	std::vector<Mesh> meshes_;

	std::vector<VulkanBuffer> modelBuffers_;

public:
	Model() = default;

	void Load(VulkanContext& ctx, const std::string& path);
	void LoadBindless(VulkanContext& ctx, 
		const std::string& path, 
		std::vector<VertexData>& globalVertices,
		std::vector<uint32_t>& globalIndices,
		uint32_t& globalVertexOffset,
		uint32_t& globalIndexOffset);

	// Destructor
	~Model();

	uint32_t GetNumTextures() { return static_cast<uint32_t>(textureList_.size()); }
	VulkanImage* GetTexture(uint32_t textureIndex);

	void AddTextureIfEmpty(TextureType tType, const std::string& filePath);

	uint32_t NumMeshes()
	{
		return static_cast<uint32_t>(meshes_.size());
	}

	void CreateModelUBOBuffers(VulkanContext& ctx);
	void SetModelUBO(VulkanContext& ctx, ModelUBO ubo);

private:
	void AddTexture(VulkanContext& ctx, const std::string& textureFilename);
	void AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height);

	// Loads a model with supported ASSIMP extensions from file and 
	// stores the resulting meshes in the meshes vector.
	void LoadModel(
		VulkanContext& ctx, 
		std::string const& path,
		std::vector<VertexData>& globalVertices,
		std::vector<uint32_t>& globalIndices,
		uint32_t& globalVertexOffset,
		uint32_t& globalIndexOffset);

	// Processes a node recursively. 
	void ProcessNode(
		VulkanContext& ctx, 
		std::vector<VertexData>& globalVertices,
		std::vector<uint32_t>& globalIndices,
		uint32_t& globalVertexOffset,
		uint32_t& globalIndexOffset,
		aiNode* node, 
		const aiScene* scene, 
		const glm::mat4& parentTransform);

	void ProcessMesh(
		VulkanContext& ctx, 
		std::vector<VertexData>& globalVertices,
		std::vector<uint32_t>& globalIndices,
		uint32_t& globalVertexOffset,
		uint32_t& globalIndexOffset,
		aiMesh* mesh, 
		const aiScene* scene, 
		const glm::mat4& transform);

	//void CreateBindlessResources(VulkanContext& ctx);
};

#endif


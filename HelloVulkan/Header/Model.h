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
	bool bindlessTexture_;
	VkDevice device_;
	std::string directory_;

	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_;

public:
	std::vector<Mesh> meshes_;

	// NOTE Textures are stored in Model regardless of bindless textures or Slot-Based
	std::vector<VulkanImage> textureList_;

	// Optional per-frame buffers for model matrix
	// TODO Maybe can be moved to pipelines
	std::vector<VulkanBuffer> modelBuffers_;

public:
	Model() = default;
	~Model() = default;

	void Destroy();

	void LoadSlotBased(VulkanContext& ctx, const std::string& path);
	void LoadBindless(VulkanContext& ctx, 
		const std::string& path, 
		std::vector<VertexData>& globalVertices,
		std::vector<uint32_t>& globalIndices,
		uint32_t& globalVertexOffset,
		uint32_t& globalIndexOffset);

	VulkanImage* GetTexture(uint32_t textureIndex);

	void AddTextureIfEmpty(TextureType tType, const std::string& filePath);

	uint32_t GetTextureCount() const { return static_cast<uint32_t>(textureList_.size()); }
	uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshes_.size()); }

	void CreateModelUBOBuffers(VulkanContext& ctx);
	void SetModelUBO(VulkanContext& ctx, ModelUBO ubo);

private:
	void AddTexture(VulkanContext& ctx, const std::string& textureFilename);
	void AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height);

	// TODO Three functios below are pretty ugly because we pass too many references
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
};

#endif


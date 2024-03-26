#ifndef MODEL
#define MODEL

#include "Mesh.h"
#include "UBOs.h"
#include "ScenePODs.h"
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
public:
	std::vector<Mesh> meshes_ = {};

	// NOTE Textures are stored in Model regardless of bindless textures or Slot-Based
	std::vector<VulkanImage> textureList_ = {};

	// Optional per-frame buffers for model matrix
	// TODO Maybe can be moved to pipelines
	std::vector<VulkanBuffer> modelBuffers_ = {};

	// This is used to store the filename and to activate instancing in bindless setup
	ModelCreateInfo modelInfo_ = {};

private:
	const aiScene* scene_ = nullptr;
	bool bindlessTexture_ = false;
	VkDevice device_ = nullptr;
	std::string directory_ = {};

	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_ = {};

public:
	Model() = default;
	~Model() = default;

	void Destroy();

	void LoadSlotBased(VulkanContext& ctx, const std::string& path);
	void LoadBindless(VulkanContext& ctx,
		const ModelCreateInfo& modelData,
		SceneData& sceneData
	);

	[[nodiscard]] VulkanImage* GetTexture(uint32_t textureIndex);
	[[nodiscard]] uint32_t GetTextureCount() const { return static_cast<uint32_t>(textureList_.size()); }
	[[nodiscard]] uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshes_.size()); }

	void CreateModelUBOBuffers(VulkanContext& ctx);
	void SetModelUBO(VulkanContext& ctx, ModelUBO ubo);

private:
	void AddTexture(VulkanContext& ctx, const std::string& textureFilename);
	void AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height);

	// Entry point
	void LoadModel(
		VulkanContext& ctx,
		std::string const& path,
		SceneData& sceneData
	);

	// Processes a node recursively
	void ProcessNode(
		VulkanContext& ctx,
		SceneData& sceneData,
		const aiNode* node,
		const glm::mat4& parentTransform);

	void ProcessMesh(
		VulkanContext& ctx,
		SceneData& sceneData,
		const aiMesh* mesh,
		const glm::mat4& transform);

	[[nodiscard]] std::vector<VertexData> GetVertices(const aiMesh* mesh, const glm::mat4& transform);
	[[nodiscard]] std::vector<uint32_t> GetIndices(const aiMesh* mesh);
	[[nodiscard]] std::unordered_map<TextureType, uint32_t> GetTextures(VulkanContext& ctx, const aiMesh* mesh);
};

#endif
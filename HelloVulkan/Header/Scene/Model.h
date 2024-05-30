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

#include "assimp/scene.h"

class Model
{
public:
	std::string filepath_{};
	std::vector<Mesh> meshes_{};

	// NOTE Textures are stored in Model regardless of bindless textures or Slot-Based
	std::vector<VulkanImage> textureList_{};

	// Optional per-frame buffers for model matrix
	// TODO Maybe can be moved to pipelines
	std::vector<VulkanBuffer> modelBuffers_{};

	// This is used to store the filename and to activate instancing in bindless setup
	ModelCreateInfo modelInfo_{};

	// Skinning
	std::unordered_map<std::string, BoneInfo> boneInfoMap_{};

private:
	const aiScene* scene_{};
	bool bindlessTexture_ = false;
	VkDevice device_{};
	std::string directory_{};

	// Skinning
	int boneCounter_ = 0;
	bool processAnimation_ = false;

	// string key is the filename, int value points to elements in textureList_
	std::unordered_map<std::string, uint32_t> textureMap_{};

public:
	Model() = default;
	~Model() = default;

	void Destroy();

	void LoadSlotBased(VulkanContext& ctx, const std::string& path);
	void LoadBindless(VulkanContext& ctx,
		const ModelCreateInfo& modelInfo,
		SceneData& sceneData
	);

	[[nodiscard]] const aiScene* GetAssimpScene() const { return scene_; }
	[[nodiscard]] VulkanImage* GetTexture(uint32_t textureIndex);
	[[nodiscard]] uint32_t GetTextureCount() const { return static_cast<uint32_t>(textureList_.size()); }
	[[nodiscard]] uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshes_.size()); }
	[[nodiscard]] int GetBoneCounter() const { return boneCounter_; }
	[[nodiscard]] int ProcessAnimation() const { return processAnimation_; }

	void CreateModelUBOBuffers(VulkanContext& ctx);
	void SetModelUBO(VulkanContext& ctx, ModelUBO ubo);

private:
	void CreateDefaultTextures(VulkanContext& ctx);
	void AddTexture(VulkanContext& ctx, const std::string& textureFilename);
	void AddTexture(VulkanContext& ctx, const std::string& textureName, void* data, int width, int height);
	[[nodiscard]] std::unordered_map<TextureType, uint32_t> GetTextureIndices(VulkanContext& ctx, const aiMesh* mesh);

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

	void SetBoneToDefault(
		std::vector<uint32_t>& skinningIndices,
		std::vector<iSVec>& boneIDArray,
		std::vector<fSVec>& boneWeightArray,
		uint32_t vertexCount,
		uint32_t prevVertexOffset);

	void ExtractBoneWeight(
		std::vector<iSVec>& boneIDs,
		std::vector<fSVec>& boneWeights,
		const aiMesh* mesh);

	[[nodiscard]] std::vector<VertexData> GetMeshVertices(const aiMesh* mesh, const glm::mat4& transform);
	[[nodiscard]] std::vector<uint32_t> GetMeshIndices(const aiMesh* mesh);
};

#endif
#ifndef SCENE_BINDLESS
#define SCENE_BINDLESS

#include "Model.h"
#include "UBO.h"

#include <vector>
#include <string>

class Scene
{
public:
	Scene(VulkanContext& ctx, const std::vector<std::string>& modelFilenames);
	~Scene();

	uint32_t GetMeshCount() { return static_cast<uint32_t>(meshDataArray_.size()); }

	std::vector<VkDescriptorImageInfo> GetImageInfos();
	std::vector<uint32_t> GetMeshVertexCountArray();

	void UpdateModelMatrix(VulkanContext& ctx, ModelUBO modelUBO, uint32_t frameIndex, uint32_t modelIndex);

private:
	void CreateBindlessResources(VulkanContext& ctx);

public:
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;
	VkDeviceSize meshDataBufferSize_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;
	VkDeviceSize vertexBufferSize_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;
	VkDeviceSize indexBufferSize_;

	// Per-frame buffer
	std::vector<VulkanBuffer> modelUBOBuffers_;
	VkDeviceSize modelUBOBufferSize_;

	std::vector<Model> models_ = {};
};

#endif
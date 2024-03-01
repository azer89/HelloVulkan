#ifndef SCENE_BINDLESS
#define SCENE_BINDLESS

#include "Model.h"
#include "UBO.h"

#include <vector>
#include <string>

/*
A scene used for bindless rendering that contains huge SSBO buffers for vertices, indices, and mesh data.
*/
class Scene
{
public:
	Scene(VulkanContext& ctx, const std::vector<std::string>& modelFilenames);
	~Scene();

	uint32_t GetMeshCount() const { return static_cast<uint32_t>(meshDataArray_.size()); }
	std::vector<VkDescriptorImageInfo> GetImageInfos() const;
	std::vector<uint32_t> GetMeshVertexCountArray() const;

	void UpdateModelMatrix(VulkanContext& ctx, 
		const ModelUBO& modelUBO, 
		uint32_t modelIndex,
		uint32_t frameIndex);
	void UpdateModelMatrix(VulkanContext& ctx,
		const ModelUBO& modelUBO,
		uint32_t modelIndex);

private:
	void CreateBindlessResources(VulkanContext& ctx);

public:
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;

	// Per-frame buffer
	std::vector<VulkanBuffer> modelSSBOBuffers_;

	std::vector<Model> models_ = {};
};

#endif
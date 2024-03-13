#ifndef SCENE_BINDLESS_TEXTURE
#define SCENE_BINDLESS_TEXTURE

#include "Model.h"
#include "UBOs.h"
#include "BoundingBox.h"
#include "VIM.h"

#include <vector>
#include <string>

/*
A scene used for indirect draw + bindless resources 
that contains huge SSBO buffers for vertices, indices, and mesh data.
*/
class Scene
{
public:
	Scene(VulkanContext& ctx, const std::vector<std::string>& modelFilenames, bool supportDeviceAddress = false);
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

	VIM GetVIM() const;

private:
	void CreateBindlessTextureResources(VulkanContext& ctx);
	void BuildBoundingBoxes(VulkanContext& ctx);

public:
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;

	std::vector<BoundingBox> boundingBoxes_;
	VulkanBuffer boundingBoxBuffer_;

	// Per-frame buffer
	std::vector<ModelUBO> modelUBOs_;
	std::vector<VulkanBuffer> modelSSBOBuffers_;

	std::vector<Model> models_ = {};

private:
	bool supportDeviceAddress_;
};

#endif
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
	VIM GetVIM() const;

	void UpdateModelMatrix(VulkanContext& ctx,
		const ModelUBO& modelUBO,
		uint32_t modelIndex);

	void CreateIndirectBuffers(
		VulkanContext& ctx,
		std::vector<VulkanBuffer>& indirectBuffers);

private:
	void CreateBindlessTextureResources(VulkanContext& ctx);

	void BuildBoundingBoxes(VulkanContext& ctx);

	void BuildModelToMeshDataMapping();

public:
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;

	std::vector<BoundingBox> originalBoundingBoxes_;
	std::vector<BoundingBox> transformedBoundingBoxes_;
	VulkanBuffer transformedBoundingBoxBuffer_;

	// Per-frame buffer
	std::vector<ModelUBO> modelUBOs_;
	std::vector<VulkanBuffer> modelSSBOBuffers_;

	std::vector<Model> models_ = {};

	// Mapping from models_ to meshDataArray_
	std::vector<std::vector<int>> modelToMeshDataMap_;

	std::vector<VulkanBuffer> indirectBuffers_;

private:
	bool supportDeviceAddress_;
};

#endif
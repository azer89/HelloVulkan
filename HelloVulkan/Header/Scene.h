#ifndef SCENE_BINDLESS_TEXTURE
#define SCENE_BINDLESS_TEXTURE

#include "Model.h"
#include "UBOs.h"
#include "BoundingBox.h"
#include "VIM.h"

#include <vector>
#include <string>

/*
A scene used for indirect draw + bindless resources that contains 
SSBO buffers for vertices, indices, and mesh data.
*/
class Scene
{
public:
	Scene(VulkanContext& ctx, const std::vector<ModelData>& modelDataArray, bool supportDeviceAddress = false);
	~Scene();

	uint32_t GetInstanceCount() const { return static_cast<uint32_t>(meshDataArray_.size()); }
	std::vector<VkDescriptorImageInfo> GetImageInfos() const;
	VIM GetVIM() const;

	void UpdateModelMatrix(VulkanContext& ctx,
		const ModelUBO& modelUBO,
		uint32_t modelIndex,
		uint32_t instanceIndex);

	void CreateIndirectBuffers(
		VulkanContext& ctx,
		std::vector<VulkanBuffer>& indirectBuffers);

private:
	void CreateBindlessResources(VulkanContext& ctx);
	void BuildBoundingBoxes(VulkanContext& ctx);
	void BuildModelToMeshDataMapping();
	std::vector<uint32_t> GetInstanceVertexCountArray() const;

public:
	// meshDataArray_ has the the same length as originalBoundingBoxes_
	std::vector<MeshData> meshDataArray_ = {};
	VulkanBuffer meshDataBuffer_;

	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;

	// For indirect draw
	std::vector<VulkanBuffer> indirectBuffers_ = {};

	// Length of modelUBO_ is instance count
	std::vector<ModelUBO> modelUBOs_ = {}; 
	std::vector<VulkanBuffer> modelSSBOBuffers_ = {}; // Frame-in-flight

	std::vector<Model> models_ = {};

	// For compute-based culling
	std::vector<BoundingBox> originalBoundingBoxes_ = {};
	std::vector<BoundingBox> transformedBoundingBoxes_ = {};
	VulkanBuffer transformedBoundingBoxBuffer_; // TODO Currently no Frame-in-flight

	// Mapping from Model to Mesh
	// Needed for UpdateModelMatrix()
	std::vector<std::vector<int>> modelToMeshMap_ = {};

	// Mapping (modelIndex, instanceIndex) --> (modelUBOs_)
	// Needed for UpdateModelMatrix()
	std::vector<std::vector<int>> modelInstanceToModelMatrix_ = {};

private:
	bool supportDeviceAddress_;
};

#endif
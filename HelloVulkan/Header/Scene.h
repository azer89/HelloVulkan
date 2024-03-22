#ifndef SCENE_BINDLESS_TEXTURE
#define SCENE_BINDLESS_TEXTURE

#include "Model.h"
#include "UBOs.h"
#include "BoundingBox.h"
#include "VIM.h"

#include <vector>
#include <span>

struct InstanceData
{
	uint32_t modelIndex;
	uint32_t meshIndex;
	uint32_t perModelInstanceIndex;
	MeshData meshData;
	BoundingBox originalBoundingBox;
};

// Needed for updating bounding boxes
struct InstanceMap
{
	uint32_t modelMatrixIndex;
	std::vector<uint32_t> instanceDataIndices;
};

/*
A scene used for indirect draw + bindless resources that contains 
SSBO buffers for vertices, indices, and mesh data.
*/
class Scene
{
public:
	Scene(
		VulkanContext& ctx,
		const std::span<ModelCreateInfo> modelDataArray,
		const bool supportDeviceAddress = false);
	~Scene();

	[[nodiscard]] uint32_t GetInstanceCount() const { return static_cast<uint32_t>(meshDataArray_.size()); }
	[[nodiscard]] std::vector<VkDescriptorImageInfo> GetImageInfos() const;
	[[nodiscard]] VIM GetVIM() const;

	void GetOffsetAndDrawCount(MaterialType matType, VkDeviceSize& offset, uint32_t& drawCount) const;

	void UpdateModelMatrix(VulkanContext& ctx,
		const ModelUBO& modelUBO,
		uint32_t modelIndex,
		uint32_t instanceIndex);

	void CreateIndirectBuffer(
		VulkanContext& ctx,
		VulkanBuffer& indirectBuffer);

private:
	void CreateBindlessResources(VulkanContext& ctx);
	void CreateDataStructures();
	[[nodiscard]] BoundingBox GetBoundingBox(uint32_t vertexStart, uint32_t vertexEnd);

public:
	std::vector<VertexData> vertices_ = {};
	VulkanBuffer vertexBuffer_;

	std::vector<uint32_t> indices_ = {};
	VulkanBuffer indexBuffer_;

	std::vector<Model> models_ = {};

	// Length of modelUBO_ is global instance count
	std::vector<ModelUBO> modelSSBOs_ = {};
	std::vector<VulkanBuffer> modelSSBOBuffers_ = {}; // Frame-in-flight

	// These three have the same length
	std::vector<InstanceData> instanceDataArray_ = {};
	std::vector<MeshData> meshDataArray_ = {}; // Content is sent to meshDataBuffer_
	std::vector<BoundingBox> transformedBoundingBoxes_ = {}; // Content is sent to transformedBoundingBoxBuffer_
	
	VulkanBuffer indirectBuffer_;
	VulkanBuffer meshDataBuffer_;
	VulkanBuffer transformedBoundingBoxBuffer_; // TODO Implement Frame-in-flight
	
private:
	bool supportDeviceAddress_;

	// First index is modelID, second index is per-model instanceID
	std::vector<std::vector<InstanceMap>> instanceMapArray_ = {};
};

#endif
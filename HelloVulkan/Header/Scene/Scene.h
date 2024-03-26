#ifndef SCENE_BINDLESS_TEXTURE
#define SCENE_BINDLESS_TEXTURE

#include "BDA.h"
#include "UBOs.h"
#include "Model.h"
#include "ScenePODs.h"
#include "BoundingBox.h"

#include <vector>
#include <span>

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
	[[nodiscard]] BDA GetBDA() const;

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
	uint32_t triangleCount_ = 0;
	SceneData sceneData_ = {}; // Containing vertices and indices
	std::vector<Model> models_ = {};
	std::vector<ModelUBO> modelSSBOs_ = {};

	// These three have the same length
	std::vector<InstanceData> instanceDataArray_ = {};
	std::vector<MeshData> meshDataArray_ = {}; // Content is sent to meshDataBuffer_
	std::vector<BoundingBox> transformedBoundingBoxes_ = {}; // Content is sent to transformedBoundingBoxBuffer_

	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;
	VulkanBuffer indirectBuffer_;
	VulkanBuffer meshDataBuffer_;
	VulkanBuffer transformedBoundingBoxBuffer_; // TODO Implement Frame-in-flight
	std::vector<VulkanBuffer> modelSSBOBuffers_ = {}; // Frame-in-flight
	
private:
	bool supportDeviceAddress_;

	// First index is modelID, second index is per-model instanceID
	std::vector<std::vector<InstanceMap>> instanceMapArray_ = {};
};

#endif
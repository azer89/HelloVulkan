#ifndef SCENE_BINDLESS_TEXTURE
#define SCENE_BINDLESS_TEXTURE

#include "BDA.h"
#include "UBOs.h"
#include "Model.h"
#include "Animation.h"
#include "Animator.h"
#include "ScenePODs.h"
#include "BoundingBox.h"

#include <vector>
#include <span>

/*
A scene used for indirect draw + bindless resources that contains SSBO buffers for vertices, indices, and mesh data.

Below is an example of a Scene structure:

Scene
|
|-- Model
|    |-- Mesh
|    `-- Mesh
|
|-- Model
|    |-- Mesh
|    |-- Mesh
|    `-- Mesh

A scene has multiple models, each model is loaded from a file (fbx, glTF, etc.), and a model has multiple meshes.
A single mesh requires one draw call. For example, if the scene has 10 meshes, 10 draw calls will be issued.
The scene representation supports instances, each is defined as a copy of a mesh.
Instances only duplicate the draw call of a mesh, so this is different than hardware instancing.

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
	[[nodiscard]] int GetClickedInstanceIndex(const Ray& ray);

	void GetOffsetAndDrawCount(MaterialType matType, VkDeviceSize& offset, uint32_t& drawCount) const;

	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	void UpdateModelMatrix(
		VulkanContext& ctx,
		const ModelUBO& modelUBO,
		const uint32_t modelIndex,
		const uint32_t perModelInstanceIndex);

	/*Only update the buffer of model matrix
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	void UpdateModelMatrixBuffer(
		VulkanContext& ctx,
		const uint32_t modelIndex,
		const uint32_t perModelInstanceIndex);
	
	void CreateIndirectBuffer(
		VulkanContext& ctx,
		VulkanBuffer& indirectBuffer);

	void UpdateAnimation(VulkanContext& ctx, float deltaTime);

private:
	[[nodiscard]] bool HasAnimation() const { return !sceneData_.boneIDArray.empty(); }

	void CreateAnimationResources(VulkanContext& ctx);
	void CreateBindlessResources(VulkanContext& ctx);
	void CreateDataStructures();
	
public:
	uint32_t triangleCount_ = 0;
	SceneData sceneData_ = {}; // Containing vertices and indices
	std::vector<ModelUBO> modelSSBOs_ = {};

	// These three have the same length
	std::vector<MeshData> meshDataArray_ = {}; // Content is sent to meshDataBuffer_
	std::vector<InstanceData> instanceDataArray_ = {};
	std::vector<BoundingBox> transformedBoundingBoxes_ = {}; // Content is sent to transformedBoundingBoxBuffer_
	
	// Animation
	VulkanBuffer boneIDBuffer_ = {};
	VulkanBuffer boneWeightBuffer_ = {};
	VulkanBuffer skinningIndicesBuffer_ = {};
	VulkanBuffer preSkinningVertexBuffer_ = {}; // This buffer contains a subset of vertexBuffer_
	std::array<VulkanBuffer, AppConfig::FrameCount> boneMatricesBuffers_ = {}; // Frame-in-flight

	// Vertex pulling
	VulkanBuffer vertexBuffer_ = {}; 
	VulkanBuffer indexBuffer_ = {};
	VulkanBuffer indirectBuffer_ = {};
	VulkanBuffer meshDataBuffer_ = {};
	std::array<VulkanBuffer, AppConfig::FrameCount> modelSSBOBuffers_ = {}; // Frame-in-flight

	// Frustum culling
	VulkanBuffer transformedBoundingBoxBuffer_ = {}; // TODO No Frame-in-flight but somenow not giving error
	
private:
	bool supportDeviceAddress_ = false;

	std::vector<Model> models_ = {};

	/*Update model matrix and update the buffer
	Need two indices to access instanceMapArray_
		First index is modelIndex
		Second index is perModelInstanceIndex*/
	std::vector<std::vector<InstanceMap>> instanceMapArray_ = {};

	// Animation
	std::vector<glm::mat4> skinningMatrices_ = {};
	std::vector<Animation> animations_ = {};
	std::vector<Animator> animators_ = {};
};

#endif
#ifndef PIPELINE_RAYTRACING
#define PIPELINE_RAYTRACING

#include "PipelineBase.h"
#include "AccelStructure.h"
#include "RTModelData.h"
#include "ShaderBindingTables.h"
#include "ShaderGroups.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "Scene.h"
#include "ResourcesLight.h"
#include "Configs.h"

#include <array>

class PipelineRaytracing final : public PipelineBase
{
public:
	PipelineRaytracing(VulkanContext& ctx, Scene* scene, ResourcesLight* resourcesLight);
	~PipelineRaytracing();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override {}
	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void OnWindowResized(VulkanContext& ctx) override;
	void ResetFrameCounter() { frameCounter_ = 0; }

	void SetRaytracingCameraUBO(
		VulkanContext& ctx,
		const glm::mat4& inverseProjection,
		const glm::mat4& inverseView,
		const glm::vec3& cameraPosition);

private:
	void CreateBLAS(VulkanContext& ctx);
	void CreateTLAS(VulkanContext& ctx);
	void CreateBDABuffer(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);
	void UpdateDescriptor(VulkanContext& ctx);
	void CreateStorageImage(VulkanContext& ctx, VkFormat imageFormat, VulkanImage* image);
	void CreateRayTracingPipeline(VulkanContext& ctx);

private:
	uint32_t frameCounter_ = 0;

	VulkanImage storageImage_ = {};
	VulkanBuffer bdaBuffer_;
	VulkanDescriptorInfo descriptorInfo_ = {};
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_ = {};

	Scene* scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;

	AccelStructure blas_ = {};
	AccelStructure tlas_ = {};
	ShaderGroups shaderGroups_ = {};
	ShaderBindingTables shaderBindingTables_ = {};
	std::vector<RTModelData> modelDataArray_ = {};
	std::vector<VkDescriptorImageInfo> textureInfoArray_ = {};
};

#endif
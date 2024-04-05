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

	void SetRaytracingCameraUBO(VulkanContext& ctx, const RaytracingCameraUBO& ubo)
	{
		const uint32_t frameIndex = ctx.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(RaytracingCameraUBO));
	}

private:
	void CreateBLAS(VulkanContext& ctx);
	void CreateTLAS(VulkanContext& ctx);
	void CreateStorageImage(VulkanContext& ctx);
	void CreateDescriptor(VulkanContext& ctx);
	void UpdateDescriptor(VulkanContext& ctx);
	void CreateRayTracingPipeline(VulkanContext& ctx);

private:
	VulkanImage storageImage_ = {};
	VulkanDescriptorInfo descriptorInfo_ = {};
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_ = {};

	Scene* scene_ = nullptr;
	ResourcesLight* resourcesLight_ = nullptr;

	AccelStructure blas_ = {};
	AccelStructure tlas_ = {};
	ShaderBindingTables sbt_ = {};
	std::vector<RTModelData> modelDataArray_ = {};
	std::vector<VkDescriptorImageInfo> textureInfoArray_ = {};
	ShaderGroups sg_ = {};
};

#endif
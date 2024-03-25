#ifndef PIPELINE_SIMPLE_RAYTRACING
#define PIPELINE_SIMPLE_RAYTRACING

#include "PipelineBase.h"
#include "AccelStructure.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "Scene.h"
#include "Configs.h"

#include <array>

class PipelineSimpleRaytracing final : public PipelineBase
{
public:
	PipelineSimpleRaytracing(VulkanContext& ctx, Scene* scene);
	~PipelineSimpleRaytracing();

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

	void CreateShaderBindingTable(VulkanContext& ctx);

private:
	VulkanImage storageImage_;
	VulkanDescriptorInfo descriptorInfo_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

	// Acceleration structures
	Scene* scene_;
	AccelStructure blas_;
	AccelStructure tlas_;
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;
	VulkanBuffer transformBuffer_;

	// Shader related
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups_;
	VulkanBuffer raygenShaderBindingTable_;
	VulkanBuffer missShaderBindingTable_;
	VulkanBuffer hitShaderBindingTable_;
};

#endif
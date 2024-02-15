#ifndef PIPELINE_SIMPLE_RAYTRACING
#define PIPELINE_SIMPLE_RAYTRACING

#include "PipelineBase.h"
#include "AccelerationStructure.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

class PipelineSimpleRaytracing final : public PipelineBase
{
public:
	PipelineSimpleRaytracing(VulkanContext& vkDev);
	~PipelineSimpleRaytracing();

	void FillCommandBuffer(VulkanContext& vkDev, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& vkDev) override;

	void SetRaytracingCameraUBO(VulkanContext& vkDev, RaytracingCameraUBO ubo)
	{
		uint32_t frameIndex = vkDev.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(RaytracingCameraUBO));
	}

private:
	void CreateBLAS(VulkanContext& vkDev);

	void CreateTLAS(VulkanContext& vkDev);

	void CreateStorageImage(VulkanContext& vkDev);

	void CreateDescriptor(VulkanContext& vkDev);

	void UpdateDescriptor(VulkanContext& vkDev);

	void CreateRayTracingPipeline(VulkanContext& vkDev);

	void CreateShaderBindingTable(VulkanContext& vkDev);

private:
	VulkanImage storageImage_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;

	// Acceleration structures
	AccelerationStructure blas_;
	AccelerationStructure tlas_;
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
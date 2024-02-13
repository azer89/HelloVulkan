#ifndef PIPELINE_SIMPLE_RAYTRACING
#define PIPELINE_SIMPLE_RAYTRACING

#include "PipelineBase.h"
#include "RaytracingAccelerationStructure.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

class PipelineSimpleRaytracing final : public PipelineBase
{
public:
	PipelineSimpleRaytracing(VulkanDevice& vkDev);
	~PipelineSimpleRaytracing();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanDevice& vkDev) override;

	void SetRaytracingCameraUBO(VulkanDevice& vkDev, RaytracingCameraUBO ubo)
	{
		uint32_t frameIndex = vkDev.GetFrameIndex();
		cameraUBOBuffers_[frameIndex].UploadBufferData(vkDev, 0, &ubo, sizeof(RaytracingCameraUBO));
	}

private:
	void CreateBLAS(VulkanDevice& vkDev);

	void CreateTLAS(VulkanDevice& vkDev);

	void CreateStorageImage(VulkanDevice& vkDev);

	void CreateDescriptor(VulkanDevice& vkDev);

	void UpdateDescriptor(VulkanDevice& vkDev);

	void CreateRayTracingPipeline(VulkanDevice& vkDev);

	void CreateShaderBindingTable(VulkanDevice& vkDev);

private:
	VulkanImage storageImage_;

	RaytracingAccelerationStructure blas_;
	RaytracingAccelerationStructure tlas_;

	std::vector<VkDescriptorSet> descriptorSets_;

	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;
	uint32_t indexCount_;
	VulkanBuffer transformBuffer_;

	// Shader related
	std::vector<VkRayTracingShaderGroupCreateInfoKHR> shaderGroups_;
	VulkanBuffer raygenShaderBindingTable_;
	VulkanBuffer missShaderBindingTable_;
	VulkanBuffer hitShaderBindingTable_;
};

#endif
#ifndef PIPELINE_SIMPLE_RAYTRACING
#define PIPELINE_SIMPLE_RAYTRACING

#include "PipelineBase.h"
#include "RaytracingStructs.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

class PipelineSimpleRaytracing final : public PipelineBase
{
public:
	PipelineSimpleRaytracing(VulkanDevice& vkDev);
	~PipelineSimpleRaytracing();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer) override;

	void SetUBOTemp(VulkanDevice& vkDev, CameraUBO ubo)
	{
		ubo.projection = glm::inverse(ubo.projection);
		ubo.view = glm::inverse(ubo.view);

		/*ubo.projection[0][0] = 1.0264;
		ubo.projection[0][1] = 0;
		ubo.projection[0][2] = -0;
		ubo.projection[0][3] = 0;
		ubo.projection[1][0] = 0;
		ubo.projection[1][1] = 0.57735;
		ubo.projection[1][2] = 0;
		ubo.projection[1][3] = -0;
		ubo.projection[2][0] = -0;
		ubo.projection[2][1] = 0;
		ubo.projection[2][2] = -0;
		ubo.projection[2][3] = -9.99805;
		ubo.projection[3][0] = 0;
		ubo.projection[3][1] = -0;
		ubo.projection[3][2] = -1;
		ubo.projection[3][3] = 10;

		ubo.view[0][0] = 1;
		ubo.view[0][1] = -0;
		ubo.view[0][2] = 0;
		ubo.view[0][3] = -0;
		ubo.view[1][0] = -0;
		ubo.view[1][1] = 1;
		ubo.view[1][2] = -0;
		ubo.view[1][3] = 0;
		ubo.view[2][0] = 0;
		ubo.view[2][1] = -0;
		ubo.view[2][2] = 1;
		ubo.view[2][3] = -0;
		ubo.view[3][0] = -0;
		ubo.view[3][1] = 0;
		ubo.view[3][2] = 2.5;
		ubo.view[3][3] = 1;*/

		cameraUboBuffer_.UploadBufferData(vkDev, 0, &ubo, sizeof(CameraUBO));
	}

private:
	void CreateBLAS(VulkanDevice& vkDev);

	void CreateTLAS(VulkanDevice& vkDev);

	void CreateStorageImage(VulkanDevice& vkDev);

	void CreateDescriptor(VulkanDevice& vkDev);

	void CreateRayTracingPipeline(VulkanDevice& vkDev);

	void CreateAccelerationStructureBuffer(
		VulkanDevice& vkDev, 
		AccelerationStructure& accelerationStructure, 
		VkAccelerationStructureBuildSizesInfoKHR buildSizeInfo);

	void CreateShaderBindingTable(VulkanDevice& vkDev);

	ScratchBuffer CreateScratchBuffer(VulkanDevice& vkDev, VkDeviceSize size);
	void DeleteScratchBuffer(VulkanDevice& vkDev, ScratchBuffer& scratchBuffer);

	uint64_t GetBufferDeviceAddress(VulkanDevice& vkDev, VkBuffer buffer);

	void TransitionImageLayoutCommand(
		VkCommandBuffer commandBuffer,
		VkImage image,
		VkFormat format,
		VkImageLayout oldLayout,
		VkImageLayout newLayout,
		uint32_t layerCount,
		uint32_t mipLevels);

private:
	VulkanImage storageImage_;

	AccelerationStructure blas_;
	AccelerationStructure tlas_;

	VulkanBuffer cameraUboBuffer_;

	VkDescriptorSet descriptorSet_;
	VkDescriptorSetLayout descriptorLayout_;
	VkDescriptorPool descriptorPool_;

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

#ifndef RENDERER_BRDF
#define RENDERER_BRDF

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"

class RendererBRDF final : RendererBase
{
public:
	RendererBRDF(
		VulkanDevice& vkDev, 
		const char* shaderName, 
		uint32_t inputSize, 
		uint32_t outputSize);

	~RendererBRDF();

	inline void Upload(VulkanDevice& vkDev, uint32_t offset, void* inData, uint32_t byteCount)
	{
		inBuffer_.UploadBufferData(vkDev, offset, inData, byteCount);
	}

	inline void Download(VulkanDevice& vkDev, uint32_t offset, void* outData, uint32_t byteCount)
	{
		outBuffer_.DownloadBufferData(vkDev, offset, outData, byteCount);
	}

	void Execute(VulkanDevice& vkDev, uint32_t xsize, uint32_t ysize, uint32_t zsize);

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

private:
	VulkanBuffer inBuffer_;
	VulkanBuffer outBuffer_;

	VkDescriptorSet descriptorSet_;

	VkPipeline pipeline_;

private:
	void CreateComputeDescriptorSetLayout(VkDevice device);
	
	void CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);

	void CreateComputePipeline(
		VkDevice device,
		VkShaderModule computeShader);

};

#endif
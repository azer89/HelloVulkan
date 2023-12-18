#ifndef COMPUTE_BASE
#define COMPUTE_BASE

#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "VulkanTexture.h"
#include "VulkanImageBarrier.h"
#include "AppSettings.h"

#include "volk.h"

#include <iostream>
#include <array>
#include <vector>

class ComputePBR
{
private:
	const static uint32_t envMapSize_ = 1024;
	const static uint32_t irradianceMapSize_ = 32;
	const static uint32_t brdfLUTSize_ = 256;
	const static VkDeviceSize uniformBufferSize = 64 * 1024;

public:
	explicit ComputePBR(VulkanDevice& vkDev, std::string hdrEnvFile) :
		device_(vkDev.GetDevice()),
		numMipmap(NumMipmap(envMapSize_, envMapSize_))
	{
		std::cout << "Init compute pipeline\n";

		CreateSampler(vkDev);
		CreateDescriptorPool();
		CreateLayouts();

		Execute(vkDev, hdrEnvFile.c_str());

		std::cout << "End compute pipeline\n";
	}

	~ComputePBR()
	{
		vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
		vkDestroySampler(device_, sampler_, nullptr);
		vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
		vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
	}

private:
	void Execute(VulkanDevice& vkDev, const char* hdrFile);

	void CreateSampler(VulkanDevice& vkDev);

	void CreateLayouts();

	void CreateDescriptorPool();

	VkDescriptorSetLayout CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>* bindings);

	VkDescriptorSet AllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout);

	VkPipelineLayout CreatePipelineLayout(
		const std::vector<VkDescriptorSetLayout>* setLayouts,
		const std::vector<VkPushConstantRange>* pushConstants);

	VkPipeline CreateComputePipeline(
		const std::string& cs,
		VkPipelineLayout layout,
		const VkSpecializationInfo* specializationInfo);

	void UpdateDescriptorSet(
		VkDescriptorSet dstSet,
		uint32_t dstBinding,
		VkDescriptorType descriptorType,
		const std::vector<VkDescriptorImageInfo>& descriptors);

	void PipelineBarrier(
		VkCommandBuffer commandBuffer,
		VkPipelineStageFlags srcStageMask,
		VkPipelineStageFlags dstStageMask,
		VkImageMemoryBarrier* barrier);

	void GenerateMipmaps(VulkanDevice& vkDev, VulkanTexture& texture);

	static int NumMipmap(int width, int height)
	{
		return static_cast<int>(floor(log2(std::max(width, height)))) + 1;
	}

private:
	uint32_t numMipmap;

	VkDevice device_ = nullptr;

	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	VkSampler sampler_;

	VkDescriptorSet descriptorSet_;
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;
};

#endif
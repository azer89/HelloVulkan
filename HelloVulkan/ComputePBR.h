#ifndef COMPUTE_BASE
#define COMPUTE_BASE

#include "VulkanDevice.h"
#include "VulkanUtility.h"
#include "VulkanTexture.h"

#include "volk.h"

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
		CreateSampler(vkDev);
		CreateDescriptorPool();
		CreateLayouts();

		VulkanTexture envTextureUnfiltered;
		envTextureUnfiltered.CreateTextureImage(vkDev, hdrEnvFile.c_str());
		envTextureUnfiltered.image_.CreateImageView(
			vkDev.GetDevice(),
			VK_FORMAT_R16G16B16A16_SFLOAT,
			VK_IMAGE_ASPECT_COLOR_BIT, // VkImageAspectFlags
			VK_IMAGE_VIEW_TYPE_CUBE, // VkImageViewType
			6,
			numMipmap);
	}

	~ComputePBR()
	{
		vkDestroyDescriptorSetLayout(device_, descriptorSetLayout_, nullptr);
		vkDestroySampler(device_, sampler_, nullptr);
		vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
		vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
	}

private:
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

	VkDescriptorSet descriptorSets_;
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;
};

#endif
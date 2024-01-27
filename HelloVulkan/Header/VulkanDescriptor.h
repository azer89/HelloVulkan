#ifndef VULKAN_DESCRIPTOR
#define VULKAN_DESCRIPTOR

#include "volk.h"

#include "VulkanDevice.h"

#include <vector>

struct DescriptorBinding
{
	VkDescriptorType descriptorType_;
	VkShaderStageFlags shaderFlags_;
	uint32_t bindingCount_;
};

struct DescriptorWrite
{
	VkDescriptorImageInfo* imageInfoPtr_;
	VkDescriptorBufferInfo* bufferInfoPtr_;
	VkDescriptorType type_;
};

struct DescriptorPoolCreateInfo
{
	uint32_t uboCount_ = 0;
	uint32_t ssboCount_ = 0;
	uint32_t samplerCount_ = 0;
	uint32_t swapchainCount_ = 0;
	uint32_t setCountPerSwapchain_ = 0;
	VkDescriptorPoolCreateFlags flags_ = 0;
};

class VulkanDescriptor
{
public:
	void CreatePool(VulkanDevice& vkDev,
		DescriptorPoolCreateInfo createInfo);

	void CreateLayout(VulkanDevice& vkDev, 
		const std::vector<DescriptorBinding>& bindings);

	void CreateSet(VulkanDevice& vkDev, const std::vector<DescriptorWrite>& writes, VkDescriptorSet* set);

	void Destroy(VkDevice device);

public:
	VkDescriptorPool pool_ = nullptr;
	VkDescriptorSetLayout layout_ = nullptr;

private:
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount = 1);

};

#endif
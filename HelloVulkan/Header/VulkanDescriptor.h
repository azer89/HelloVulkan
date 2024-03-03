#ifndef VULKAN_DESCRIPTOR
#define VULKAN_DESCRIPTOR

#include "VulkanContext.h"
#include "VulkanDescriptorInfo.h"

#include <vector>

class VulkanDescriptor
{
public:
	void CreatePoolAndLayout(
		VulkanContext& ctx, 
		const VulkanDescriptorInfo& descriptorInfo,
		uint32_t frameCount,
		uint32_t setCountPerFrame,
		VkDescriptorPoolCreateFlags poolFlags = 0);

	void CreateSet(VulkanContext& ctx, const VulkanDescriptorInfo& descriptorInfo, VkDescriptorSet* set);

	void AllocateSet(VulkanContext& ctx, VkDescriptorSet* set);

	void UpdateSet(VulkanContext& ctx, const VulkanDescriptorInfo& descriptorInfo, VkDescriptorSet* set);

	void Destroy();

private:
	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount);

public:
	VkDescriptorPool pool_ = nullptr;
	VkDescriptorSetLayout layout_ = nullptr;

private:
	VkDevice device_ = nullptr;
};

#endif
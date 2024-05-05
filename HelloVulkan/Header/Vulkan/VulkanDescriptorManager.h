#ifndef VULKAN_DESCRIPTOR_MANAGER
#define VULKAN_DESCRIPTOR_MANAGER

#include "VulkanContext.h"
#include "VulkanDescriptorSetInfo.h"

/*
Manages descriptor pool and layout.
Also creates descriptor sets.
*/
class VulkanDescriptorManager
{
public:
	void CreatePoolAndLayout(
		VulkanContext& ctx, 
		const VulkanDescriptorSetInfo& descriptorInfo,
		uint32_t frameCount,
		uint32_t setCountPerFrame,
		VkDescriptorPoolCreateFlags poolFlags = 0);

	void CreateSet(VulkanContext& ctx, const VulkanDescriptorSetInfo& descriptorInfo, VkDescriptorSet* set);

	void AllocateSet(VulkanContext& ctx, VkDescriptorSet* set);

	void UpdateSet(VulkanContext& ctx, const VulkanDescriptorSetInfo& descriptorInfo, VkDescriptorSet* set);

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
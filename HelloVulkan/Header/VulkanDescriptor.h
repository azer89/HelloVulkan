#ifndef VULKAN_DESCRIPTOR
#define VULKAN_DESCRIPTOR

#include "VulkanContext.h"
#include "DescriptorBuildInfo.h"

#include <vector>

class VulkanDescriptor
{
public:
	void CreatePoolAndLayout(
		VulkanContext& ctx, 
		const DescriptorBuildInfo& buildInfo,
		uint32_t frameCount,
		uint32_t setCountPerFrame,
		VkDescriptorPoolCreateFlags poolFlags = 0);

	void CreateSet(VulkanContext& ctx, const std::vector<DescriptorSetWrite>& writes, VkDescriptorSet* set);

	void AllocateSet(VulkanContext& ctx, VkDescriptorSet* set);

	void UpdateSet(VulkanContext& ctx, const std::vector<DescriptorSetWrite>& writes, VkDescriptorSet* set);

	void Destroy();

public:
	VkDescriptorPool pool_ = nullptr;
	VkDescriptorSetLayout layout_ = nullptr;

private:
	VkDescriptorSetLayoutBinding CreateDescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount);

private:
	VkDevice device_ = nullptr;
};

#endif
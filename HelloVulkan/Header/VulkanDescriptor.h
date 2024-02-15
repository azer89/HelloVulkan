#ifndef VULKAN_DESCRIPTOR
#define VULKAN_DESCRIPTOR

#include "volk.h"

#include "VulkanContext.h"

#include <vector>

struct DescriptorBinding
{
	VkDescriptorType descriptorType_;
	VkShaderStageFlags shaderFlags_;
	uint32_t bindingCount_;
};

struct DescriptorWrite
{
	void* pNext_ = nullptr;
	VkDescriptorImageInfo* imageInfoPtr_ = nullptr;
	VkDescriptorBufferInfo* bufferInfoPtr_ = nullptr;
	VkDescriptorType type_;
};

struct DescriptorPoolCreateInfo
{
	uint32_t uboCount_ = 0;
	uint32_t ssboCount_ = 0;
	uint32_t samplerCount_ = 0;

	// These two below are only for raytracing pipeline
	uint32_t storageImageCount_ = 0;
	uint32_t accelerationStructureCount_ = 0;

	// For duplication, because there are overlapping frames
	uint32_t frameCount_ = 0; 

	uint32_t setCountPerFrame_ = 0;
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

	void AllocateSet(VulkanDevice& vkDev, VkDescriptorSet* set);

	void UpdateSet(VulkanDevice& vkDev, const std::vector<DescriptorWrite>& writes, VkDescriptorSet* set);

	void Destroy();

public:
	VkDescriptorPool pool_ = nullptr;
	VkDescriptorSetLayout layout_ = nullptr;

private:
	VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(
		uint32_t binding,
		VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags,
		uint32_t descriptorCount = 1);

private:
	VkDevice device_ = nullptr;
};

#endif
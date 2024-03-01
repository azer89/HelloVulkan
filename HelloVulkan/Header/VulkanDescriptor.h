#ifndef VULKAN_DESCRIPTOR
#define VULKAN_DESCRIPTOR

#include "VulkanContext.h"
#include "DescriptorBuildInfo.h"

#include <vector>

// For creating descriptor layout
struct DescriptorLayoutBinding
{
	VkDescriptorType type_;
	VkShaderStageFlags shaderFlags_;
	uint32_t descriptorCount_ = 1u;
	uint32_t bindingCount_ = 1u;
};

// For creating descriptor set
/*struct DescriptorSetWrite
{
	// pNext_ is for raytracing pipeline
	void* pNext_ = nullptr;
	VkDescriptorImageInfo* imageInfoPtr_ = nullptr;
	VkDescriptorBufferInfo* bufferInfoPtr_ = nullptr;
	// If you have an array of buffers/images, descriptorCount_ must be bigger than one
	uint32_t descriptorCount_ = 1u;
	VkDescriptorType type_;
	VkShaderStageFlags shaderFlags_;
};*/

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
	void CreatePool(VulkanContext& ctx,
		DescriptorPoolCreateInfo createInfo);

	void CreateLayout(VulkanContext& ctx, 
		const std::vector<DescriptorLayoutBinding>& bindings);

	void CreatePoolAndLayout(
		VulkanContext& ctx, 
		//const std::vector<DescriptorSetWrite>& writes,
		DescriptorBuildInfo buildInfo,
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
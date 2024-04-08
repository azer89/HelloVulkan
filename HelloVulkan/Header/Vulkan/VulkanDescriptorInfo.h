#ifndef DESCRIPTOR_BUILD_INFO
#define DESCRIPTOR_BUILD_INFO

#include "volk.h"

#include <vector>
#include <unordered_map>

class VulkanBuffer;
class VulkanImage;

struct DescriptorWrite
{
	// pNext_ is for raytracing pipeline
	void* pNext_ = nullptr;
	VkDescriptorImageInfo* imageInfoPtr_ = nullptr;
	VkDescriptorBufferInfo* bufferInfoPtr_ = nullptr;
	// If you have an array of buffers/images, descriptorCount_ must be bigger than one
	uint32_t descriptorCount_ = 1u;
	VkDescriptorType descriptorType_;
	VkShaderStageFlags shaderStage_;
};

class VulkanDescriptorInfo
{
public:
	std::vector<DescriptorWrite> writes_;

private:
	std::unordered_map<size_t, VkDescriptorBufferInfo> bufferMap_;
	std::unordered_map<size_t, VkDescriptorImageInfo> imageMap_;

	// Descriptor indexing
	std::vector<VkDescriptorImageInfo> imageArrays_;

public:
	void AddBuffer(
		const VulkanBuffer* buffer,
		VkDescriptorType dsType,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddImage(
		const VulkanImage* image,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
	void UpdateBuffer(const VulkanBuffer* buffer, size_t bindingIndex);
	void UpdateImage(const VulkanImage* image, size_t bindingIndex);

	// Descriptor indexing
	// TODO imageArrays is not stored so you need to supply it again if you update the descriptors
	void AddImageArray(
		const std::vector<VkDescriptorImageInfo>& imageArray,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);

	// Raytracing
	void AddAccelerationStructure(VkShaderStageFlags stageFlags);
	void AddAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo, VkShaderStageFlags stageFlags);
	void UpdateStorageImage(const VulkanImage* image, size_t bindingIndex);
	// TODO asInfo is not stored so you need to supply it again if you update the descriptors
	void UpdateAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo, size_t bindingIndex);

private:
	void CheckBound(size_t bindingIndex) const;
};

#endif
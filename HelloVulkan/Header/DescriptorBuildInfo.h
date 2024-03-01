#ifndef DESCRIPTOR_BUILD_INFO
#define DESCRIPTOR_BUILD_INFO

#include "volk.h"

#include <vector>
#include <unordered_map>

class VulkanBuffer;
class VulkanImage;

struct DescriptorSetWrite
{
	// pNext_ is for raytracing pipeline
	void* pNext_ = nullptr;
	VkDescriptorImageInfo* imageInfoPtr_ = nullptr;
	VkDescriptorBufferInfo* bufferInfoPtr_ = nullptr;
	// If you have an array of buffers/images, descriptorCount_ must be bigger than one
	uint32_t descriptorCount_ = 1u;
	VkDescriptorType type_;
	VkShaderStageFlags shaderFlags_; // TODO rename to stageFlags_
};

class DescriptorBuildInfo
{
public:
	std::vector<DescriptorSetWrite> writes_;

private:
	std::unordered_map<size_t, VkDescriptorBufferInfo> bufferMap_;
	std::unordered_map<size_t, VkDescriptorImageInfo> imageMap_;

	// Descriptor indexing
	std::vector<VkDescriptorImageInfo> imageArrays_;

public:
	void AddBuffer(
		VulkanBuffer* buffer,
		VkDescriptorType dsType,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	void AddImage(
		VulkanImage* image,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);
	void UpdateBuffer(VulkanBuffer* buffer, size_t bindingIndex);
	void UpdateImage(VulkanImage* image, size_t bindingIndex);

	// Descriptor indexing
	void AddImageArray(
		const std::vector<VkDescriptorImageInfo>& imageArrays,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT);

	// Raytracing
	void AddAccelerationStructure();
	void AddAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo);
	void UpdateStorageImage(VulkanImage* image, size_t bindingIndex);
	void UpdateAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo, size_t bindingIndex);

private:
	void CheckBound(size_t bindingIndex);
};

#endif
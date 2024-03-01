#include "DescriptorBuildInfo.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include <iostream>

void DescriptorBuildInfo::AddBuffer(
	VulkanBuffer* buffer,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		size_t index = writes_.size();
		bufferMap_[index] = buffer->GetBufferInfo();
		bufferInfo = &(bufferMap_[index]);
	}

	writes_.push_back
	({
		.bufferInfoPtr_ = bufferInfo,
		.descriptorCount_ = 1u,
		.type_ = dsType,
		.shaderFlags_ = stageFlags
		});
}

void DescriptorBuildInfo::AddImage(
	VulkanImage* image,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		size_t index = writes_.size();
		imageMap_[index] = image->GetDescriptorImageInfo();
		imageInfo = &(imageMap_[index]);
	}

	writes_.push_back
	({
		.imageInfoPtr_ = imageInfo,
		.descriptorCount_ = 1u,
		.type_ = dsType,
		.shaderFlags_ = stageFlags
		});
}

// Special case for descriptor indexing
void DescriptorBuildInfo::AddImageArray(
	const std::vector<VkDescriptorImageInfo>& imageArrays,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags)
{
	imageArrays_ = imageArrays;

	writes_.push_back
	({
		.imageInfoPtr_ = imageArrays_.data(),
		.descriptorCount_ = static_cast<uint32_t>(imageArrays_.size()),
		.type_ = dsType,
		.shaderFlags_ = stageFlags
		});
}

// Raytracing
void DescriptorBuildInfo::AddAccelerationStructure()
{
	writes_.push_back
	({
		.descriptorCount_ = 1u,
		.type_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
		});
}

// Raytracing
void DescriptorBuildInfo::AddAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo)
{
	writes_.push_back
	({
		.pNext_ = asInfo,
		.descriptorCount_ = 1u,
		.type_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
		});
}

void DescriptorBuildInfo::UpdateBuffer(VulkanBuffer* buffer, size_t bindingIndex)
{
	if (bindingIndex < 0 || bindingIndex >= writes_.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}

	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		bufferMap_[bindingIndex] = buffer->GetBufferInfo();
		bufferInfo = &(bufferMap_[bindingIndex]);
	}

	writes_[bindingIndex].bufferInfoPtr_ = bufferInfo;
}

void DescriptorBuildInfo::UpdateImage(VulkanImage* image, size_t bindingIndex)
{
	if (bindingIndex < 0 || bindingIndex >= writes_.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}

	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		imageMap_[bindingIndex] = image->GetDescriptorImageInfo();
		imageInfo = &(imageMap_[bindingIndex]);
	}

	writes_[bindingIndex].imageInfoPtr_ = imageInfo;
}

// Special case for raytracing
void DescriptorBuildInfo::UpdateStorageImage(VulkanImage* image, size_t bindingIndex)
{
	if (bindingIndex < 0 || bindingIndex >= writes_.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}

	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		imageMap_[bindingIndex] =
		{
			.imageView = image->imageView_,
			.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
		};
		imageInfo = &(imageMap_[bindingIndex]);
	}

	writes_[bindingIndex].imageInfoPtr_ = imageInfo;
}

// Special case for raytracing
void DescriptorBuildInfo::UpdateAccelerationStructure(
	VkWriteDescriptorSetAccelerationStructureKHR* asInfo, 
	size_t bindingIndex)
{
	if (bindingIndex < 0 || bindingIndex >= writes_.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}
	writes_[bindingIndex].pNext_ = asInfo;
}
#include "DescriptorBuildInfo.h"
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include <iostream>

void DescriptorBuildInfo::AddBuffer(
	const VulkanBuffer* buffer,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		const size_t index = writes_.size();
		bufferMap_[index] = buffer->GetBufferInfo();
		bufferInfo = &(bufferMap_[index]);
	}

	writes_.push_back
	({
		.bufferInfoPtr_ = bufferInfo,
		.descriptorCount_ = 1u,
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

void DescriptorBuildInfo::AddImage(
	const VulkanImage* image,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags
)
{
	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		const size_t index = writes_.size();
		imageMap_[index] = image->GetDescriptorImageInfo();
		imageInfo = &(imageMap_[index]);
	}

	writes_.push_back
	({
		.imageInfoPtr_ = imageInfo,
		.descriptorCount_ = 1u,
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

// Special case for descriptor indexing
void DescriptorBuildInfo::AddImageArray(
	const std::vector<VkDescriptorImageInfo>& imageArray,
	VkDescriptorType dsType,
	VkShaderStageFlags stageFlags)
{
	imageArrays_ = imageArray;

	writes_.push_back
	({
		.imageInfoPtr_ = imageArrays_.data(),
		.descriptorCount_ = static_cast<uint32_t>(imageArrays_.size()),
		.descriptorType_ = dsType,
		.shaderStage_ = stageFlags
		});
}

// Raytracing
void DescriptorBuildInfo::AddAccelerationStructure()
{
	writes_.push_back
	({
		.descriptorCount_ = 1u,
		.descriptorType_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.shaderStage_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
		});
}

// Raytracing
void DescriptorBuildInfo::AddAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo)
{
	writes_.push_back
	({
		.pNext_ = asInfo,
		.descriptorCount_ = 1u,
		.descriptorType_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
		.shaderStage_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
		});
}

void DescriptorBuildInfo::UpdateBuffer(const VulkanBuffer* buffer, size_t bindingIndex)
{
	CheckBound(bindingIndex);

	VkDescriptorBufferInfo* bufferInfo = nullptr;
	if (buffer)
	{
		bufferMap_[bindingIndex] = buffer->GetBufferInfo();
		bufferInfo = &(bufferMap_[bindingIndex]);
	}

	writes_[bindingIndex].bufferInfoPtr_ = bufferInfo;
}

void DescriptorBuildInfo::UpdateImage(const VulkanImage* image, size_t bindingIndex)
{
	CheckBound(bindingIndex);

	VkDescriptorImageInfo* imageInfo = nullptr;
	if (image)
	{
		imageMap_[bindingIndex] = image->GetDescriptorImageInfo();
		imageInfo = &(imageMap_[bindingIndex]);
	}

	writes_[bindingIndex].imageInfoPtr_ = imageInfo;
}

// Special case for raytracing
void DescriptorBuildInfo::UpdateStorageImage(const VulkanImage* image, size_t bindingIndex)
{
	CheckBound(bindingIndex);

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
	CheckBound(bindingIndex);
	writes_[bindingIndex].pNext_ = asInfo;
}

void DescriptorBuildInfo::CheckBound(size_t bindingIndex) const
{
	if (bindingIndex < 0 || bindingIndex >= writes_.size())
	{
		std::cerr << "Invalid bindingIndex\n";
	}
}
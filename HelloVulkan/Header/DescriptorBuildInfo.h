#ifndef DESCRIPTOR_BUILD_INFO
#define DESCRIPTOR_BUILD_INFO

#include "volk.h"

// TODO Create a .cpp and remove these
#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include <vector>
#include <unordered_map>
#include <iostream>

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

	// Special case for descriptor indexing
	std::vector<VkDescriptorImageInfo> imageArrays_;

public:

	void AddBuffer(
		VulkanBuffer* buffer,
		VkDescriptorType dsType,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
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

	void AddImage(
		VulkanImage* image,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
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
	void AddImageArray(
		const std::vector<VkDescriptorImageInfo>& imageArrays,
		VkDescriptorType dsType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VkShaderStageFlags stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT)
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
	void AddAccelerationStructure()
	{
		writes_.push_back
		({
			.descriptorCount_ = 1u,
			.type_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
			});
	}

	// Raytracing
	void AddAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo)
	{
		writes_.push_back
		({
			.pNext_ = asInfo,
			.descriptorCount_ = 1u,
			.type_ = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.shaderFlags_ = VK_SHADER_STAGE_RAYGEN_BIT_KHR
			});
	}

	void UpdateBuffer(VulkanBuffer* buffer, size_t bindingIndex)
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

	void UpdateImage(VulkanImage* image, size_t bindingIndex)
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
	void UpdateStorageImage(VulkanImage* image, size_t bindingIndex)
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
	void UpdateAccelerationStructure(VkWriteDescriptorSetAccelerationStructureKHR* asInfo, size_t bindingIndex)
	{
		if (bindingIndex < 0 || bindingIndex >= writes_.size())
		{
			std::cerr << "Invalid bindingIndex\n";
		}
		writes_[bindingIndex].pNext_ = asInfo;
	}
};

#endif
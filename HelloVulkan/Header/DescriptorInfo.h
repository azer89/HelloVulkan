#ifndef DESCRIPTOR_INFO
#define DESCRIPTOR_INFO

#include "volk.h"

#include "VulkanBuffer.h"
#include "VulkanImage.h"

#include <vector>
#include <unordered_map>
#include <iostream>

struct WriteInfo
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
	std::vector<WriteInfo> writes_;

private:
	std::unordered_map<size_t, VkDescriptorBufferInfo> bufferMap_;
	std::unordered_map<size_t, VkDescriptorImageInfo> imageMap_;

public:

	void AddBuffer(
		VulkanBuffer* buffer,
		VkDescriptorType dsType,
		uint32_t descriptorCount = 1u,
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
			.descriptorCount_ = descriptorCount,
			.type_ = dsType,
			.shaderFlags_ = stageFlags
		});
	}

	void AddImage(
		VulkanImage* image,
		VkDescriptorType dsType,
		uint32_t descriptorCount,
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
			.descriptorCount_ = descriptorCount,
			.type_ = dsType,
			.shaderFlags_ = stageFlags
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
};

#endif
#include "VulkanDescriptor.h"
#include "VulkanCheck.h"

#include <iostream>

void VulkanDescriptor::CreatePoolAndLayout(
	VulkanContext& ctx, 
	const VulkanDescriptorInfo& descriptorInfo,
	uint32_t frameCount,
	uint32_t setCountPerFrame,
	VkDescriptorPoolCreateFlags poolFlags)
{
	device_ = ctx.GetDevice();

	uint32_t uboCount = 0;
	uint32_t ssboCount = 0;
	uint32_t samplerCount = 0;
	uint32_t storageImageCount = 0;
	uint32_t accelerationStructureCount = 0;

	// Create pool
	for (auto& write : descriptorInfo.writes_)
	{
		if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			uboCount += write.descriptorCount_;
		}
		else if(write.descriptorType_ == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			ssboCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			samplerCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
		{
			accelerationStructureCount += write.descriptorCount_;
		}
		else if (write.descriptorType_ == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			storageImageCount += write.descriptorCount_;
		}
		else
		{
			std::cerr << "Descriptor type is currently not supported\n";
		}
	}

	// Frame-in-flight
	uboCount *= frameCount;
	ssboCount *= frameCount;
	samplerCount *= frameCount;
	storageImageCount *= frameCount;
	accelerationStructureCount *= frameCount;

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (uboCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = uboCount
			});
	}

	if (ssboCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = ssboCount
			});
	}

	if (samplerCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = samplerCount
			});
	}

	if (storageImageCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = storageImageCount
			});
	}

	if (accelerationStructureCount)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
				.descriptorCount = accelerationStructureCount
			});
	}

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = poolFlags,
		.maxSets = static_cast<uint32_t>(frameCount * setCountPerFrame),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(ctx.GetDevice(), &poolInfo, nullptr, &pool_));
	
	// Create Layout
	std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
	uint32_t bindingIndex = 0;
	for (auto& write : descriptorInfo.writes_)
	{
		layoutBindings.emplace_back(
			CreateDescriptorSetLayoutBinding
			(
				bindingIndex++,
				write.descriptorType_,
				write.shaderStage_,
				write.descriptorCount_
			)
		);
	}
	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(layoutBindings.size()),
		.pBindings = layoutBindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(ctx.GetDevice(), &layoutInfo, nullptr, &layout_));
}

void VulkanDescriptor::CreateSet(
	VulkanContext& ctx, 
	const VulkanDescriptorInfo& descriptorInfo,
	VkDescriptorSet* set)
{
	AllocateSet(ctx, set);

	UpdateSet(ctx, descriptorInfo, set);
}

void VulkanDescriptor::AllocateSet(VulkanContext& ctx, VkDescriptorSet* set)
{
	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = pool_,
		.descriptorSetCount = 1u,
		.pSetLayouts = &layout_
	};

	VK_CHECK(vkAllocateDescriptorSets(ctx.GetDevice(), &allocInfo, set));
}

void VulkanDescriptor::UpdateSet(
	VulkanContext& ctx, 
	const VulkanDescriptorInfo& descriptorInfo,
	VkDescriptorSet* set)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	uint32_t bindIndex = 0;

	descriptorWrites.reserve(descriptorInfo.writes_.size());
	for (auto& write : descriptorInfo.writes_)
	{
		descriptorWrites.push_back({
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = write.pNext_,
			.dstSet = *set, // Dereference
			.dstBinding = bindIndex++,
			.dstArrayElement = 0,
			.descriptorCount = write.descriptorCount_,
			.descriptorType = write.descriptorType_,
			.pImageInfo = write.imageInfoPtr_,
			.pBufferInfo = write.bufferInfoPtr_,
			.pTexelBufferView = nullptr
		});
	}

	vkUpdateDescriptorSets
	(
		ctx.GetDevice(),
		static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(),
		0,
		nullptr
	);
}

void VulkanDescriptor::Destroy()
{
	if (layout_)
	{
		vkDestroyDescriptorSetLayout(device_, layout_, nullptr);
		layout_ = nullptr;
	}

	if (pool_)
	{
		vkDestroyDescriptorPool(device_, pool_, nullptr);
		pool_ = nullptr;
	}
}

VkDescriptorSetLayoutBinding VulkanDescriptor::CreateDescriptorSetLayoutBinding(
	uint32_t binding,
	VkDescriptorType descriptorType,
	VkShaderStageFlags stageFlags,
	uint32_t descriptorCount)
{
	return VkDescriptorSetLayoutBinding
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}
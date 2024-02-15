#include "VulkanDescriptor.h"
#include "VulkanUtility.h"

void VulkanDescriptor::CreatePool(VulkanDevice& vkDev,
	DescriptorPoolCreateInfo createInfo)
{
	std::vector<VkDescriptorPoolSize> poolSizes;

	if (createInfo.uboCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = createInfo.frameCount_ * createInfo.uboCount_
			});
	}

	if (createInfo.ssboCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = createInfo.frameCount_ * createInfo.ssboCount_
			});
	}

	if (createInfo.samplerCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = createInfo.frameCount_ * createInfo.samplerCount_
			});
	}

	if (createInfo.storageImageCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = createInfo.frameCount_ * createInfo.storageImageCount_
			});
	}

	if (createInfo.accelerationStructureCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
				.descriptorCount = createInfo.frameCount_ * createInfo.accelerationStructureCount_
			});
	}

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = createInfo.flags_,
		.maxSets = static_cast<uint32_t>(createInfo.frameCount_ * createInfo.setCountPerFrame_),
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.data()
	};

	VK_CHECK(vkCreateDescriptorPool(vkDev.GetDevice(), &poolInfo, nullptr, &pool_));
}

void VulkanDescriptor::CreateLayout(VulkanDevice& vkDev,
	const std::vector<DescriptorBinding>& bindings)
{
	std::vector<VkDescriptorSetLayoutBinding> vulkanBindings;

	uint32_t bindingIndex = 0;
	for (size_t i = 0; i < bindings.size(); ++i)
	{
		for (size_t j = 0; j < bindings[i].bindingCount_; ++j)
		{
			vulkanBindings.emplace_back(
				DescriptorSetLayoutBinding
				(
					bindingIndex++,
					bindings[i].descriptorType_,
					bindings[i].shaderFlags_
				)
			);
		}
	}

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(vulkanBindings.size()),
		.pBindings = vulkanBindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &layout_));
}

void VulkanDescriptor::CreateSet(
	VulkanDevice& vkDev, 
	const std::vector<DescriptorWrite>& writes,
	VkDescriptorSet* set)
{
	AllocateSet(vkDev, set);

	UpdateSet(vkDev, writes, set);
}

void VulkanDescriptor::AllocateSet(VulkanDevice& vkDev, VkDescriptorSet* set)
{
	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = pool_,
		.descriptorSetCount = 1u,
		.pSetLayouts = &layout_
	};

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, set));
}

void VulkanDescriptor::UpdateSet(VulkanDevice& vkDev, const std::vector<DescriptorWrite>& writes, VkDescriptorSet* set)
{
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	uint32_t bindIndex = 0;

	for (size_t i = 0; i < writes.size(); ++i)
	{
		descriptorWrites.push_back({
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.pNext = writes[i].pNext_,
			.dstSet = *set, // Dereference
			.dstBinding = bindIndex++,
			.dstArrayElement = 0,
			.descriptorCount = 1,
			.descriptorType = writes[i].type_,
			.pImageInfo = writes[i].imageInfoPtr_,
			.pBufferInfo = writes[i].bufferInfoPtr_,
			.pTexelBufferView = nullptr
		});
	}

	vkUpdateDescriptorSets
	(
		vkDev.GetDevice(),
		static_cast<uint32_t>(descriptorWrites.size()),
		descriptorWrites.data(),
		0,
		nullptr
	);
}

void VulkanDescriptor::Destroy(VkDevice device)
{
	vkDestroyDescriptorSetLayout(device, layout_, nullptr);
	vkDestroyDescriptorPool(device, pool_, nullptr);
}

VkDescriptorSetLayoutBinding VulkanDescriptor::DescriptorSetLayoutBinding(
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
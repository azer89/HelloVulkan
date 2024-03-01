#include "VulkanDescriptor.h"
#include "VulkanUtility.h"

void VulkanDescriptor::CreatePool(VulkanContext& ctx,
	DescriptorPoolCreateInfo createInfo)
{
	device_ = ctx.GetDevice();

	std::vector<VkDescriptorPoolSize> poolSizes;

	if (createInfo.uboCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.descriptorCount = createInfo.uboCount_
			});
	}

	if (createInfo.ssboCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.descriptorCount = createInfo.ssboCount_
			});
	}

	if (createInfo.samplerCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = createInfo.samplerCount_
			});
	}

	if (createInfo.storageImageCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
				.descriptorCount = createInfo.storageImageCount_
			});
	}

	if (createInfo.accelerationStructureCount_)
	{
		poolSizes.push_back(VkDescriptorPoolSize
			{
				.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
				.descriptorCount = createInfo.accelerationStructureCount_
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

	VK_CHECK(vkCreateDescriptorPool(ctx.GetDevice(), &poolInfo, nullptr, &pool_));
}

void VulkanDescriptor::CreateLayout(VulkanContext& ctx,
	const std::vector<DescriptorLayoutBinding>& bindings)
{
	std::vector<VkDescriptorSetLayoutBinding> vulkanBindings;

	uint32_t bindingIndex = 0;
	for (size_t i = 0; i < bindings.size(); ++i)
	{
		for (size_t j = 0; j < bindings[i].bindingCount_; ++j)
		{
			vulkanBindings.emplace_back(
				CreateDescriptorSetLayoutBinding
				(
					bindingIndex++,
					bindings[i].type_,
					bindings[i].shaderFlags_,
					bindings[i].descriptorCount_
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

	VK_CHECK(vkCreateDescriptorSetLayout(ctx.GetDevice(), &layoutInfo, nullptr, &layout_));
}

void VulkanDescriptor::CreatePoolAndLayout(
	VulkanContext& ctx, 
	//const std::vector<DescriptorSetWrite>& writes,
	DescriptorBuildInfo buildInfo,
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
	for (auto& write : buildInfo.writes_)
	{
		if (write.type_ == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			uboCount++;
		}
		else if(write.type_ == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		{
			ssboCount++;
		}
		else if (write.type_ == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			samplerCount++;
		}
		else if (write.type_ == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR)
		{
			accelerationStructureCount++;
		}
		else if (write.type_ == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			storageImageCount++;
		}
		else
		{
			std::cerr << "Descriptor type is currently not supported\n";
		}
	}

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
	for (auto& write : buildInfo.writes_)
	{
		layoutBindings.emplace_back(
			CreateDescriptorSetLayoutBinding
			(
				bindingIndex++,
				write.type_,
				write.shaderFlags_,
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
	const std::vector<DescriptorSetWrite>& writes,
	VkDescriptorSet* set)
{
	AllocateSet(ctx, set);

	UpdateSet(ctx, writes, set);
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

void VulkanDescriptor::UpdateSet(VulkanContext& ctx, const std::vector<DescriptorSetWrite>& writes, VkDescriptorSet* set)
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
			.descriptorCount = writes[i].descriptorCount_,
			.descriptorType = writes[i].type_,
			.pImageInfo = writes[i].imageInfoPtr_,
			.pBufferInfo = writes[i].bufferInfoPtr_,
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
	}

	if (pool_)
	{
		vkDestroyDescriptorPool(device_, pool_, nullptr);
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
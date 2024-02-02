#include "VulkanShader.h"

#include "RendererCullLights.h"
#include "Configs.h"

#include <array>
#include <iostream>

RendererCullLights::RendererCullLights(VulkanDevice& vkDev, Lights* lights, ClusterForwardBuffers* cfBuffers) :
	RendererBase(vkDev, true),
	lights_(lights),
	cfBuffers_(cfBuffers)
{
	// Per frame UBO
	CreateUniformBuffers(vkDev, cfUBOBuffers_, sizeof(ClusterForwardUBO));

	std::string shaderFile = AppConfig::ShaderFolder + "CullLightsBatch.comp";
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	CreateDescriptorPool(
		vkDev,
		1u,
		5u,
		0u,
		1u, // decsriptor count per swapchain
		&descriptorPool_);
	CreateComputeDescriptorLayout(vkDev);
	CreateComputeDescriptorSets(vkDev);
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);
	CreateComputePipeline(vkDev, shader.GetShaderModule());

	shader.Destroy(vkDev.GetDevice());
}

RendererCullLights::~RendererCullLights()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy(device_);
	}
	//clusterForwardUBO_.Destroy(device_);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void RendererCullLights::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererCullLights::FillComputeCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets_[currentImage],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	// Tell the GPU to do some compute
	//vkCmdDispatch(commandBuffer,
	//	static_cast<uint32_t>(ClusterForwardConfig::sliceCountX), // groupCountX
	//	static_cast<uint32_t>(ClusterForwardConfig::sliceCountY), // groupCountY
	//	static_cast<uint32_t>(ClusterForwardConfig::sliceCountZ)); // groupCountZ
	
	vkCmdDispatch(commandBuffer,
		1, // groupCountX
		1, // groupCountY
		6); // groupCountZ

	VkBufferMemoryBarrier lightGridBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = vkDev.GetComputeFamily(),
		.dstQueueFamilyIndex = vkDev.GetGraphicsFamily(),
		.buffer = cfBuffers_->lightCellsBuffers_[currentImage].buffer_,
		.offset = 0,
		.size = VK_WHOLE_SIZE
	};

	VkBufferMemoryBarrier lightIndicesBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = vkDev.GetComputeFamily(),
		.dstQueueFamilyIndex = vkDev.GetGraphicsFamily(),
		.buffer = cfBuffers_->lightIndicesBuffers_[currentImage].buffer_,
		.offset = 0,
		.size = VK_WHOLE_SIZE,
	};

	std::array<VkBufferMemoryBarrier, 2> barriers = 
	{ 
		lightGridBarrier, 
		lightIndicesBarrier 
	};

	vkCmdPipelineBarrier(commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
		0, 
		0, 
		nullptr, 
		barriers.size(),
		barriers.data(), 
		0, 
		nullptr);
}

void RendererCullLights::CreateComputeDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings =
	{
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		vkDev.GetDevice(),
		&descriptorSetLayoutCreateInfo,
		0,
		&descriptorSetLayout_));
}

void RendererCullLights::CreateComputeDescriptorSets(VulkanDevice& vkDev)
{
	size_t swapchainLength = vkDev.GetSwapchainImageCount();
	std::vector<VkDescriptorSetLayout> layouts(swapchainLength, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapchainLength),
		.pSetLayouts = layouts.data() //&descriptorSetLayout_
	};

	descriptorSets_.resize(swapchainLength);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapchainLength; ++i)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo1 = { cfBuffers_->aabbBuffer_.buffer_, 0, VK_WHOLE_SIZE };
		const VkDescriptorBufferInfo bufferInfo2 = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize()};
		const VkDescriptorBufferInfo bufferInfo3 = { cfBuffers_->globalIndexCountBuffers_[i].buffer_, 0, sizeof(uint32_t)};
		const VkDescriptorBufferInfo bufferInfo4 = { cfBuffers_->lightCellsBuffers_[i].buffer_, 0, VK_WHOLE_SIZE};
		const VkDescriptorBufferInfo bufferInfo5 = { cfBuffers_->lightIndicesBuffers_[i].buffer_, 0, VK_WHOLE_SIZE};
		const VkDescriptorBufferInfo bufferInfo6 = { cfUBOBuffers_[i].buffer_, 0, sizeof(ClusterForwardUBO)};

		const std::array<VkWriteDescriptorSet, 6> descriptorWrites = {
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo1,
				0u,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo2,
				1u,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo3,
				2u,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo4,
				3u,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo5,
				4u,
				VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			BufferWriteDescriptorSet(
				ds,
				&bufferInfo6,
				5u,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		};

		vkUpdateDescriptorSets(
			vkDev.GetDevice(),
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}
}

void RendererCullLights::CreateComputePipeline(
	VulkanDevice& vkDev,
	VkShaderModule computeShader)
{
	VkComputePipelineCreateInfo computePipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.stage = {  // ShaderStageInfo, just like in graphics pipeline, but with a single COMPUTE stage
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.stage = VK_SHADER_STAGE_COMPUTE_BIT,
			.module = computeShader,
			.pName = "main",
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	VK_CHECK(vkCreateComputePipelines(vkDev.GetDevice(), 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));
}
#include "VulkanShader.h"

#include "RendererAABB.h"
#include "Configs.h"

RendererAABB::RendererAABB(
	VulkanDevice& vkDev) :
	RendererBase(vkDev, true)
{
	// Create UBO Buffer
	cfUBOBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		sizeof(ClusterForwardUBO),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::string shaderFile = AppConfig::ShaderFolder + "AABB.comp";
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	CreateComputeDescriptorSetLayout(vkDev.GetDevice());
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);
	CreateComputePipeline(vkDev.GetDevice(), shader.GetShaderModule());

	shader.Destroy(vkDev.GetDevice());
}

RendererAABB::~RendererAABB()
{
	cfUBOBuffer_.Destroy(device_);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void RendererAABB::CreateClusters(VulkanDevice& vkDev, const ClusterForwardUBO& ubo, VulkanBuffer* aabbBuffer)
{
	CreateComputeDescriptorSet(vkDev.GetDevice(), aabbBuffer);
	UpdateUniformBuffer(vkDev.GetDevice(), cfUBOBuffer_, &ubo, sizeof(ClusterForwardUBO));
	Execute(vkDev, aabbBuffer);
}

void RendererAABB::Execute(VulkanDevice& vkDev, VulkanBuffer* aabbBuffer)
{
	VkCommandBuffer commandBuffer = vkDev.GetComputeCommandBuffer();

	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		0,
		VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		0
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&computeDescriptorSet_,
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	// Tell the GPU to do some compute
	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountZ)); // groupCountZ

	VkBufferMemoryBarrier barrierInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = vkDev.GetComputeFamily(),
		.dstQueueFamilyIndex = vkDev.GetGraphicsFamily(),
		.buffer = aabbBuffer->buffer_,
		.offset = 0,
		.size = VK_WHOLE_SIZE };
	vkCmdPipelineBarrier(commandBuffer, 
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, //srcStageMask
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // dstStageMask
		0, 
		0, 
		nullptr, 
		1, 
		&barrierInfo,
		0, nullptr);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = 0,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = 0,
		.pWaitDstStageMask = 0,
		.commandBufferCount = 1, // commandBufferCount
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = 0
	};

	VK_CHECK(vkQueueSubmit(vkDev.GetComputeQueue(), 1, &submitInfo, 0));

	// Wait for the compute shader completion.
	// Alternatively we can use a fence.
	VK_CHECK(vkQueueWaitIdle(vkDev.GetComputeQueue()));
}

void RendererAABB::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
	// Empty
}

void RendererAABB::CreateComputeDescriptorSetLayout(VkDevice device)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] =
	{
		// SSBO
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		// UBO
		{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo =
	{
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		0,
		0,
		2,
		descriptorSetLayoutBindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(
		device,
		&descriptorSetLayoutCreateInfo,
		0,
		&descriptorSetLayout_));
}

void RendererAABB::CreateComputeDescriptorSet(VkDevice device, VulkanBuffer* aabbBuffer)
{
	// Descriptor pool
	std::vector<VkDescriptorPoolSize> poolSizes =
	{
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1
		},
		VkDescriptorPoolSize
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1
		}
	};

	const VkDescriptorPoolCreateInfo poolInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.maxSets = 1u,
		.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
		.pPoolSizes = poolSizes.empty() ? nullptr : poolSizes.data()
	};

	// Descriptor pool
	VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool_));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = 0,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = 1,
		.pSetLayouts = &descriptorSetLayout_
	};

	// Descriptor set
	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &computeDescriptorSet_));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo aabbBufferInfo = { aabbBuffer->buffer_, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo uboBufferInfo = { cfUBOBuffer_.buffer_, 0, sizeof(ClusterForwardUBO) };

	VkWriteDescriptorSet writeDescriptorSet[2] = {
		BufferWriteDescriptorSet(
			computeDescriptorSet_,
			&aabbBufferInfo,
			0, // dstBinding
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		BufferWriteDescriptorSet(
			computeDescriptorSet_,
			&uboBufferInfo,
			1, // dstBinding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
	};

	vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);
}

void RendererAABB::CreateComputePipeline(
	VkDevice device,
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
			// we don't use specialization
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	// no caching, single pipeline creation
	VK_CHECK(vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));
}
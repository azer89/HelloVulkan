#include "RendererBRDFLUT.h"
#include "VulkanShader.h"
#include "Configs.h"

struct PushConstantsBRDFLUT
{
	uint32_t width;
	uint32_t height;
	uint32_t sampleCount;
};

RendererBRDFLUT::RendererBRDFLUT(
	VulkanDevice& vkDev) :
	RendererBase(vkDev, true)
{
	inBuffer_.CreateSharedBuffer(vkDev, sizeof(float),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	outBuffer_.CreateSharedBuffer(vkDev, IBLConfig::LUTBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::string shaderFile = AppConfig::ShaderFolder + "BRDFLUT.comp";
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	// Push constants
	std::vector<VkPushConstantRange> ranges(1u);
	VkPushConstantRange& range = ranges.front();
	range.offset = 0u;
	range.size = sizeof(PushConstantsBRDFLUT);
	range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	CreateComputeDescriptorSetLayout(vkDev.GetDevice());
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_, ranges);
	CreateComputePipeline(vkDev.GetDevice(), shader.GetShaderModule());
	CreateComputeDescriptorSet(vkDev.GetDevice(), descriptorSetLayout_);

	shader.Destroy(vkDev.GetDevice());
}

RendererBRDFLUT::~RendererBRDFLUT()
{
	inBuffer_.Destroy(device_);
	outBuffer_.Destroy(device_);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void RendererBRDFLUT::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererBRDFLUT::CreateLUT(VulkanDevice& vkDev, VulkanImage* outputLUT)
{
	std::vector<float> lutData(IBLConfig::LUTBufferSize, 0);

	Execute(vkDev);

	outBuffer_.DownloadBufferData(vkDev, 0, lutData.data(), IBLConfig::LUTBufferSize);

	outputLUT->CreateImageFromData(
		vkDev,
		lutData.data(),
		IBLConfig::LUTWidth,
		IBLConfig::LUTHeight,
		1, // Mipmap count
		1, // Layer count
		VK_FORMAT_R32G32_SFLOAT);

	outputLUT->CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);

	outputLUT->CreateDefaultSampler(
		vkDev.GetDevice()
	);
}

void RendererBRDFLUT::Execute(VulkanDevice& vkDev)
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

	PushConstantsBRDFLUT pc =
	{
		.width = IBLConfig::LUTWidth,
		.height = IBLConfig::LUTHeight,
		.sampleCount = IBLConfig::LUTSampleCount
	};
	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_COMPUTE_BIT,
		0,
		sizeof(PushConstantsBRDFLUT), &pc);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSet_,
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	// Tell the GPU to do some compute
	vkCmdDispatch(commandBuffer, 
		static_cast<uint32_t>(IBLConfig::LUTWidth), // groupCountX
		static_cast<uint32_t>(IBLConfig::LUTHeight), // groupCountY
		1u); // groupCountZ

	// We have to create a memory barrier efore the CPU can read back data.
	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT
	};

	vkCmdPipelineBarrier(
		commandBuffer, 
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
		VK_PIPELINE_STAGE_HOST_BIT, // dstStageMask
		0, // dependencyFlags
		1, // memoryBarrierCount
		&readoutBarrier, 
		0, // bufferMemoryBarrierCount 
		nullptr, 
		0, // imageMemoryBarrierCount
		nullptr);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		0, 
		0, 
		0, 
		0, 
		1, // commandBufferCount
		&commandBuffer, 
		0, 
		0
	};

	VK_CHECK(vkQueueSubmit(vkDev.GetComputeQueue(), 1, &submitInfo, 0));

	// Wait for the compute shader completion.
	// Alternatively we can use a fence.
	VK_CHECK(vkQueueWaitIdle(vkDev.GetComputeQueue()));
}

void RendererBRDFLUT::CreateComputeDescriptorSetLayout(VkDevice device)
{
	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] =
	{
		{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 },
		{ 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, 0 }
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

void RendererBRDFLUT::CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	// Descriptor pool
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		0,
		0,
		1,
		1,
		&descriptorPoolSize
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, 0, &descriptorPool_));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		0,
		descriptorPool_,
		1,
		&descriptorSetLayout_
	};

	// Descriptor set
	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet_));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo = { inBuffer_.buffer_, 0, VK_WHOLE_SIZE };
	VkDescriptorBufferInfo outBufferInfo = { outBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet writeDescriptorSet[2] = {
		BufferWriteDescriptorSet(
			descriptorSet_,
			&inBufferInfo,
			0, // dstBinding
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
		BufferWriteDescriptorSet(
			descriptorSet_,
			&outBufferInfo,
			1, // dstBinding
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
	};

	vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);
}

void RendererBRDFLUT::CreateComputePipeline(
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
#include "RendererBRDF.h"
#include "VulkanShader.h"
#include "AppSettings.h"

// TODO Use push constants to send the image dimension
constexpr int LUT_WIDTH = 256;
constexpr int LUT_HEIGHT = 256;
constexpr uint32_t BUFFER_SIZE = 2 * sizeof(float) * LUT_WIDTH * LUT_HEIGHT;

RendererBRDF::RendererBRDF(
	VulkanDevice& vkDev) :
	RendererBase(vkDev, {})
{
	inBuffer_.CreateSharedBuffer(vkDev, sizeof(float),
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	outBuffer_.CreateSharedBuffer(vkDev, BUFFER_SIZE,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::string shaderFile = AppSettings::ShaderFolder + "BRDFLUT.comp";
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	CreateComputeDescriptorSetLayout(vkDev.GetDevice());
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);
	CreateComputePipeline(vkDev.GetDevice(), shader.GetShaderModule());
	CreateComputeDescriptorSet(vkDev.GetDevice(), descriptorSetLayout_);

	shader.Destroy(vkDev.GetDevice());
}

RendererBRDF::~RendererBRDF()
{
	inBuffer_.Destroy(device_);
	outBuffer_.Destroy(device_);
	vkDestroyPipeline(device_, pipeline_, nullptr);
}

void RendererBRDF::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void RendererBRDF::CreateLUT(VulkanDevice& vkDev, VulkanTexture* outputLUT)
{
	std::vector<float> lutData(BUFFER_SIZE, 0);

	Execute(vkDev);

	outBuffer_.DownloadBufferData(vkDev, 0, lutData.data(), BUFFER_SIZE);

	outputLUT->image_.CreateImageFromData(
		vkDev,
		&lutData[0],
		LUT_WIDTH,
		LUT_HEIGHT,
		1,
		1,
		VK_FORMAT_R32G32_SFLOAT);

	outputLUT->image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT
	);
	outputLUT->CreateTextureSampler(
		vkDev.GetDevice()
	);
}

void RendererBRDF::Execute(VulkanDevice& vkDev)
{
	VkCommandBuffer commandBuffer = vkDev.GetComputeCommandBuffer();

	VkCommandBufferBeginInfo commandBufferBeginInfo = {
		VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		0, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, 0
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo));

	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0,
		1,
		&descriptorSet_,
		0,
		0);

	vkCmdDispatch(commandBuffer, 
		static_cast<uint32_t>(LUT_WIDTH), 
		static_cast<uint32_t>(LUT_HEIGHT),
		1u);

	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT
	};

	vkCmdPipelineBarrier(
		commandBuffer, 
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 
		VK_PIPELINE_STAGE_HOST_BIT, 
		0, 
		1, 
		&readoutBarrier, 
		0, 
		nullptr, 
		0, 
		nullptr);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {
		VK_STRUCTURE_TYPE_SUBMIT_INFO,
		0, 
		0, 
		0, 
		0, 
		1, 
		&commandBuffer, 
		0, 
		0
	};

	VK_CHECK(vkQueueSubmit(vkDev.GetComputeQueue(), 1, &submitInfo, 0));
	VK_CHECK(vkQueueWaitIdle(vkDev.GetComputeQueue()));
}

void RendererBRDF::CreateComputeDescriptorSetLayout(VkDevice device)
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

void RendererBRDF::CreateComputeDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
	// Descriptor pool
	VkDescriptorPoolSize descriptorPoolSize = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2 };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1, 1, &descriptorPoolSize
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, 0, &descriptorPool_));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		0, descriptorPool_, 1, &descriptorSetLayout_
	};

	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &descriptorSet_));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo = { inBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	VkDescriptorBufferInfo outBufferInfo = { outBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	VkWriteDescriptorSet writeDescriptorSet[2] = {
		{ 
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
			0, 
			descriptorSet_, 
			0, 
			0, 
			1, 
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			0,  
			&inBufferInfo, 0 },
		{ 
			VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 
			0, 
			descriptorSet_, 
			1, 
			0, 
			1, 
			VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
			0, 
			&outBufferInfo, 
			0 
		}
	};

	vkUpdateDescriptorSets(device, 2, writeDescriptorSet, 0, 0);
}

void RendererBRDF::CreateComputePipeline(
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
			/* we don't use specialization */
			.pSpecializationInfo = nullptr
		},
		.layout = pipelineLayout_,
		.basePipelineHandle = 0,
		.basePipelineIndex = 0
	};

	/* no caching, single pipeline creation*/
	VK_CHECK(vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &pipeline_));
}
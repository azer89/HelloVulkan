#include "PipelineBRDFLUT.h"
#include "VulkanShader.h"
#include "Configs.h"

struct PushConstantsBRDFLUT
{
	uint32_t width;
	uint32_t height;
	uint32_t sampleCount;
};

PipelineBRDFLUT::PipelineBRDFLUT(
	VulkanDevice& vkDev) :
	PipelineBase(vkDev, PipelineFlags::Compute)
{
	outBuffer_.CreateSharedBuffer(vkDev, IBLConfig::LUTBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	std::string shaderFile = AppConfig::ShaderFolder + "BRDFLUT.comp";
	VulkanShader shader;
	shader.Create(vkDev.GetDevice(), shaderFile.c_str());

	// Push constants
	std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
		.offset = 0u,
		.size = sizeof(PushConstantsBRDFLUT)
	}};

	CreateComputeDescriptorSetLayout(vkDev);
	CreateComputeDescriptorSet(vkDev);
	CreatePipelineLayout(vkDev.GetDevice(), descriptor_.layout_, &pipelineLayout_, ranges);
	CreateComputePipeline(vkDev.GetDevice(), shader.GetShaderModule());

	shader.Destroy(vkDev.GetDevice());
}

PipelineBRDFLUT::~PipelineBRDFLUT()
{
	outBuffer_.Destroy(device_);
}

void PipelineBRDFLUT::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage)
{
}

void PipelineBRDFLUT::CreateLUT(VulkanDevice& vkDev, VulkanImage* outputLUT)
{
	std::vector<float> lutData(IBLConfig::LUTBufferSize, 0);

	Execute(vkDev);

	// Copy the buffer content to an image
	// TODO Find a way so that compute shader can write to an image
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

void PipelineBRDFLUT::Execute(VulkanDevice& vkDev)
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

	// https://github.com/KhronosGroup/Vulkan-Docs/wiki/Synchronization-Examples-(Legacy-synchronization-APIs)#cpu-read-back-of-data-written-by-a-compute-shader
	VkMemoryBarrier readoutBarrier = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT, // Write access to SSBO
		.dstAccessMask = VK_ACCESS_HOST_READ_BIT // Read SSBO by a host / CPU
	};

	vkCmdPipelineBarrier(
		commandBuffer, // commandBuffer
		// Compute shader
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, // srcStageMask
		// Host / CPU
		VK_PIPELINE_STAGE_HOST_BIT, // dstStageMask
		0, // dependencyFlags
		1, // memoryBarrierCount
		&readoutBarrier, // pMemoryBarriers
		0, // bufferMemoryBarrierCount 
		nullptr, // pBufferMemoryBarriers
		0, // imageMemoryBarrierCount
		nullptr); // pImageMemoryBarriers

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

void PipelineBRDFLUT::CreateComputeDescriptorSetLayout(VulkanDevice& vkDev)
{
	descriptor_.CreateLayout(vkDev,
	{
		{
			.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
			.bindingCount_ = 1
		}
	});
}

void PipelineBRDFLUT::CreateComputeDescriptorSet(VulkanDevice& vkDev)
{
	// Descriptor pool
	// TODO Can be moved to constructor
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 0u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.swapchainCount_ = 1u,
			.setCountPerSwapchain_ = 1u
		});

	// Descriptor set
	VkDescriptorBufferInfo outBufferInfo = { outBuffer_.buffer_, 0, VK_WHOLE_SIZE };

	descriptor_.CreateSet(
		vkDev, 
		{
			{.bufferInfoPtr_ = &outBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER }
		},
		&descriptorSet_);
}
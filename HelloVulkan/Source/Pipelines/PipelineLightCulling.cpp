#include "PipelineLightCulling.h"
#include "Configs.h"

#include <array>

PipelineLightCulling::PipelineLightCulling(
	VulkanDevice& vkDev,
	Lights* lights,
	ClusterForwardBuffers* cfBuffers) :
	PipelineBase(vkDev,
		{
			.type_ = PipelineType::Compute
		}),
	lights_(lights),
	cfBuffers_(cfBuffers)
{
	CreateUniformBuffers(vkDev, cfUBOBuffers_, sizeof(ClusterForwardUBO));
	CreateDescriptor(vkDev);
	CreatePipelineLayout(vkDev, descriptor_.layout_, &pipelineLayout_);
	CreateComputePipeline(vkDev, AppConfig::ShaderFolder + "ClusteredForward/LightCulling.comp");
}

PipelineLightCulling::~PipelineLightCulling()
{
	for (auto uboBuffer : cfUBOBuffers_)
	{
		uboBuffer.Destroy(device_);
	}
}

void PipelineLightCulling::FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer)
{
	//uint32_t zeroValue = 0u;
	uint32_t swapchainImageIndex = vkDev.GetCurrentSwapchainImageIndex();
	//cfBuffers_->globalIndexCountBuffers_[swapchainImageIndex].UploadBufferData(vkDev, 0, &zeroValue, sizeof(uint32_t));
	Execute(vkDev, commandBuffer, swapchainImageIndex);
}

void PipelineLightCulling::Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, uint32_t swapchainImageIndex)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_COMPUTE,
		pipelineLayout_,
		0, // firstSet
		1, // descriptorSetCount
		&descriptorSets_[swapchainImageIndex],
		0, // dynamicOffsetCount
		0); // pDynamicOffsets

	vkCmdDispatch(commandBuffer,
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountX), // groupCountX
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountY), // groupCountY
		static_cast<uint32_t>(ClusterForwardConfig::sliceCountZ)); // groupCountZ

	VkBufferMemoryBarrier lightGridBarrier =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
		.pNext = nullptr,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
		.srcQueueFamilyIndex = vkDev.GetComputeFamily(),
		.dstQueueFamilyIndex = vkDev.GetGraphicsFamily(),
		.buffer = cfBuffers_->lightCellsBuffers_[swapchainImageIndex].buffer_,
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
		.buffer = cfBuffers_->lightIndicesBuffers_[swapchainImageIndex].buffer_,
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

void PipelineLightCulling::CreateDescriptor(VulkanDevice& vkDev)
{
	uint32_t imageCount = static_cast<uint32_t>(vkDev.GetSwapchainImageCount());

	// Pool
	descriptor_.CreatePool(
		vkDev,
		{
			.uboCount_ = 5u,
			.ssboCount_ = 1u,
			.samplerCount_ = 0u,
			.swapchainCount_ = imageCount,
			.setCountPerSwapchain_ = 1u
		});

	// Layout
	descriptor_.CreateLayout(vkDev,
		{
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
				.bindingCount_ = 5
			},
			{
				.descriptorType_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.shaderFlags_ = VK_SHADER_STAGE_COMPUTE_BIT,
				.bindingCount_ = 1
			}
		});

	// Set
	descriptorSets_.resize(imageCount);
	for (size_t i = 0; i < imageCount; ++i)
	{
		VkDescriptorBufferInfo bufferInfo1 = { cfBuffers_->aabbBuffers_[i].buffer_, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo bufferInfo2 = { lights_->GetSSBOBuffer(), 0, lights_->GetSSBOSize()};
		VkDescriptorBufferInfo bufferInfo3 = { cfBuffers_->globalIndexCountBuffers_[i].buffer_, 0, sizeof(uint32_t) };
		VkDescriptorBufferInfo bufferInfo4 = { cfBuffers_->lightCellsBuffers_[i].buffer_, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo bufferInfo5 = { cfBuffers_->lightIndicesBuffers_[i].buffer_, 0, VK_WHOLE_SIZE };
		VkDescriptorBufferInfo bufferInfo6 = { cfUBOBuffers_[i].buffer_, 0, sizeof(ClusterForwardUBO) };

		descriptor_.CreateSet(
			vkDev,
			{
				{.bufferInfoPtr_ = &bufferInfo1, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo2, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo3, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo4, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo5, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
				{.bufferInfoPtr_ = &bufferInfo6, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER }
			},
			&descriptorSets_[i]);
	}
}
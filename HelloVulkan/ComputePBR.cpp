#include "ComputePBR.h"
#include "VulkanShader.h"
#include "VulkanImageBarrier.h"

enum ComputeDescriptorSetBindingNames : uint32_t
{
	Binding_InputTexture = 0,
	Binding_OutputTexture = 1,
	Binding_OutputMipTail = 2,
};

struct SpecularFilterPushConstants
{
	uint32_t level;
	float roughness;
};

void ComputePBR::Execute(
	VulkanDevice& vkDev,
	const char* hdrFile
)
{
	std::cout << "Execute\n";

	VulkanTexture envTextureUnfiltered;
	envTextureUnfiltered.CreateTexture
	(
		vkDev,
		envMapSize_,
		envMapSize_,
		6,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		0,
		VK_IMAGE_USAGE_STORAGE_BIT
	);
	{
		VkPipeline pipeline = CreateComputePipeline(
			AppSettings::ShaderFolder + "equirect2cube.comp",
			pipelineLayout_,
			nullptr);

		VulkanTexture envTextureEquirect;
		envTextureEquirect.CreateHDRImage(vkDev, hdrFile);
		envTextureEquirect.image_.CreateImageView(
			vkDev.GetDevice(),
			VK_FORMAT_R32G32B32A32_SFLOAT,
			VK_IMAGE_ASPECT_COLOR_BIT);

		const VkDescriptorImageInfo inputTexture = {VK_NULL_HANDLE, envTextureEquirect.image_.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};
		const VkDescriptorImageInfo outputTexture = { VK_NULL_HANDLE, envTextureUnfiltered.image_.imageView, VK_IMAGE_LAYOUT_GENERAL };
		
		UpdateDescriptorSet(descriptorSet_, Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {inputTexture});
		UpdateDescriptorSet(descriptorSet_, Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, { outputTexture });

		VkCommandBuffer commandBuffer = vkDev.GetComputeCommandBuffer();
		BeginImmediateCommandBuffer(vkDev);
		{
			const auto preDispatchBarrier = 
				VulkanImageBarrier(
					envTextureUnfiltered.image_, 
					0, 
					VK_ACCESS_SHADER_WRITE_BIT, 
					VK_IMAGE_LAYOUT_UNDEFINED, 
					VK_IMAGE_LAYOUT_GENERAL).mipLevels(0, 1);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, { preDispatchBarrier });

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vkCmdBindDescriptorSets(
				commandBuffer, 
				VK_PIPELINE_BIND_POINT_COMPUTE, 
				pipelineLayout_, 
				0, 
				1, 
				&descriptorSet_, 
				0, 
				nullptr);
			vkCmdDispatch(commandBuffer, envMapSize_ / 32, envMapSize_ / 32, 6);

			const auto postDispatchBarrier = 
				VulkanImageBarrier(envTextureUnfiltered.image_, 
					VK_ACCESS_SHADER_WRITE_BIT, 
					0, 
					VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(0, 1);
			PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { postDispatchBarrier });
		}
		ExecuteImmediateCommandBuffer(vkDev);

		GenerateMipmaps(vkDev, envTextureUnfiltered);
		
		// Destroy resources
		envTextureEquirect.DestroyVulkanTexture(vkDev.GetDevice());

		vkDestroyPipeline(vkDev.GetDevice(), pipeline, nullptr);
	}

	envTextureUnfiltered.DestroyVulkanTexture(vkDev.GetDevice());

	std::cout << "Done\n";
}

void ComputePBR::CreateSampler(VulkanDevice& vkDev)
{
	VkSamplerCreateInfo createInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	// Linear, non-anisotropic sampler, wrap address mode (post processing compute shaders)
	createInfo.minFilter = VK_FILTER_LINEAR;
	createInfo.magFilter = VK_FILTER_LINEAR;
	createInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
	VK_CHECK(vkCreateSampler(vkDev.GetDevice(), &createInfo, nullptr, &sampler_));
}


void ComputePBR::CreateLayouts()
{
	const std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings =
	{
		{ Binding_InputTexture, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_COMPUTE_BIT, &sampler_ },
		{ Binding_OutputTexture, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
		{ Binding_OutputMipTail, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numMipmap - 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
	};

	descriptorSetLayout_ = CreateDescriptorSetLayout(&descriptorSetLayoutBindings);
	descriptorSet_ = AllocateDescriptorSet(descriptorPool_, descriptorSetLayout_);

	const std::vector<VkDescriptorSetLayout> pipelineSetLayouts = 
	{
		descriptorSetLayout_,
	};
	const std::vector<VkPushConstantRange> pipelinePushConstantRanges = 
	{
		{ VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SpecularFilterPushConstants) },
	};
	pipelineLayout_ = CreatePipelineLayout(&pipelineSetLayouts, &pipelinePushConstantRanges);
}


void ComputePBR::CreateDescriptorPool()
{
	const std::array<VkDescriptorPoolSize, 2> poolSizes = { {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numMipmap },
	} };

	VkDescriptorPoolCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	createInfo.maxSets = 2;
	createInfo.poolSizeCount = (uint32_t)poolSizes.size();
	createInfo.pPoolSizes = poolSizes.data();
	VK_CHECK(vkCreateDescriptorPool(device_, &createInfo, nullptr, &descriptorPool_));
}

VkDescriptorSetLayout ComputePBR::CreateDescriptorSetLayout(const std::vector<VkDescriptorSetLayoutBinding>* bindings)
{
	VkDescriptorSetLayout layout;
	VkDescriptorSetLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	if (bindings && bindings->size() > 0)
	{
		createInfo.bindingCount = (uint32_t)bindings->size();
		createInfo.pBindings = bindings->data();
	}
	VK_CHECK(vkCreateDescriptorSetLayout(device_, &createInfo, nullptr, &layout));
	return layout;
}

VkDescriptorSet ComputePBR::AllocateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout)
{
	VkDescriptorSet descriptorSet;
	VkDescriptorSetAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	allocateInfo.descriptorPool = pool;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &layout;
	VK_CHECK(vkAllocateDescriptorSets(device_, &allocateInfo, &descriptorSet));
	return descriptorSet;
}

VkPipelineLayout ComputePBR::CreatePipelineLayout(
	const std::vector<VkDescriptorSetLayout>* setLayouts,
	const std::vector<VkPushConstantRange>* pushConstants)
{
	VkPipelineLayout layout;
	VkPipelineLayoutCreateInfo createInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	if (setLayouts && setLayouts->size() > 0)
	{
		createInfo.setLayoutCount = (uint32_t)setLayouts->size();
		createInfo.pSetLayouts = setLayouts->data();
	}
	if (pushConstants && pushConstants->size() > 0)
	{
		createInfo.pushConstantRangeCount = (uint32_t)pushConstants->size();
		createInfo.pPushConstantRanges = pushConstants->data();
	}

	VK_CHECK(vkCreatePipelineLayout(device_, &createInfo, nullptr, &layout));

	return layout;
}

VkPipeline ComputePBR::CreateComputePipeline(
	const std::string& computeShaderFile,
	VkPipelineLayout layout,
	const VkSpecializationInfo* specializationInfo)
{
	VulkanShader computeShader;
	VK_CHECK(computeShader.Create(device_, computeShaderFile.c_str()));
	VkShaderStageFlagBits stage = GLSLangShaderStageToVulkan(GLSLangShaderStageFromFileName(computeShaderFile.c_str()));
	VkPipelineShaderStageCreateInfo shaderStage = computeShader.GetShaderStageInfo(stage, "main");
	shaderStage.pSpecializationInfo = specializationInfo;
	
	VkComputePipelineCreateInfo createInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
	createInfo.stage = shaderStage;
	createInfo.layout = layout;
	
	VkPipeline pipeline;
	VK_CHECK(vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline))
	computeShader.Destroy(device_);
	
	return pipeline;
}

void ComputePBR::UpdateDescriptorSet(
	VkDescriptorSet dstSet,
	uint32_t dstBinding,
	VkDescriptorType descriptorType,
	const std::vector<VkDescriptorImageInfo>& descriptors)
{
	VkWriteDescriptorSet writeDescriptorSet = { VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.dstBinding = dstBinding;
	writeDescriptorSet.descriptorType = descriptorType;
	writeDescriptorSet.descriptorCount = (uint32_t)descriptors.size();
	writeDescriptorSet.pImageInfo = descriptors.data();
	vkUpdateDescriptorSets(device_, 1, &writeDescriptorSet, 0, nullptr);
}

void ComputePBR::BeginImmediateCommandBuffer(VulkanDevice& vkDev)
{
	VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	VK_CHECK(vkBeginCommandBuffer(vkDev.GetComputeCommandBuffer(), &beginInfo))
}

void ComputePBR::ExecuteImmediateCommandBuffer(VulkanDevice& vkDev)
{
	VkCommandBuffer commandBuffer = vkDev.GetComputeCommandBuffer();

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(vkDev.GetComputeQueue(), 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkDev.GetComputeQueue());

	VK_CHECK(vkResetCommandBuffer(commandBuffer, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

void ComputePBR::PipelineBarrier(
	VkCommandBuffer commandBuffer,
	VkPipelineStageFlags srcStageMask,
	VkPipelineStageFlags dstStageMask,
	const std::vector<VulkanImageBarrier>& barriers)
{
	vkCmdPipelineBarrier(
		commandBuffer,
		srcStageMask,
		dstStageMask,
		0,
		0,
		nullptr,
		0,
		nullptr,
		(uint32_t)barriers.size(),
		reinterpret_cast<const VkImageMemoryBarrier*>(barriers.data()));
}

void ComputePBR::GenerateMipmaps(VulkanDevice& vkDev, VulkanTexture& texture)
{
	assert(texture.mipmapLevels_ > 1);

	VkCommandBuffer commandBuffer = vkDev.GetComputeCommandBuffer();
	BeginImmediateCommandBuffer(vkDev);

	// Iterate through mip chain and consecutively blit from previous level to next level with linear filtering.
	for (uint32_t level = 1, prevLevelWidth = texture.width_, prevLevelHeight = texture.height_; level < texture.mipmapLevels_; ++level, prevLevelWidth /= 2, prevLevelHeight /= 2)
	{

		const auto preBlitBarrier = VulkanImageBarrier(
			texture.image_,
			0,
			VK_ACCESS_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL).mipLevels(level, 1);
		PipelineBarrier(
			commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			{ preBlitBarrier });

		VkImageBlit region = {};
		region.srcSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level - 1, 0, texture.layers_ };
		region.dstSubresource = { VK_IMAGE_ASPECT_COLOR_BIT, level,   0, texture.layers_ };
		region.srcOffsets[1] = { int32_t(prevLevelWidth),  int32_t(prevLevelHeight),   1 };
		region.dstOffsets[1] = { int32_t(prevLevelWidth / 2),int32_t(prevLevelHeight / 2), 1 };
		vkCmdBlitImage(commandBuffer,
			texture.image_.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			texture.image_.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region, VK_FILTER_LINEAR);

		const auto postBlitBarrier =
			VulkanImageBarrier(
				texture.image_,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL).mipLevels(level, 1);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, { postBlitBarrier });
	}

	// Transition whole mip chain to shader read only layout.
	{
		const auto barrier =
			VulkanImageBarrier(
				texture.image_,
				VK_ACCESS_TRANSFER_WRITE_BIT,
				0,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		PipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, { barrier });
	}

	ExecuteImmediateCommandBuffer(vkDev);
}
#include "RendererCube.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

#include "glm/glm.hpp"
#include "glm/ext.hpp"

#include <cmath>
#include <array>

inline VkDescriptorSetLayoutBinding DescriptorSetLayoutBinding(uint32_t binding, VkDescriptorType descriptorType, VkShaderStageFlags stageFlags, uint32_t descriptorCount = 1)
{
	return VkDescriptorSetLayoutBinding{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = nullptr
	};
}

inline VkWriteDescriptorSet BufferWriteDescriptorSet(
	VkDescriptorSet ds, 
	const VkDescriptorBufferInfo* bi, 
	uint32_t bindIdx, 
	VkDescriptorType dType)
{
	return VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		ds, bindIdx, 0, 1, dType, nullptr, bi, nullptr
	};
}

inline VkWriteDescriptorSet ImageWriteDescriptorSet(
	VkDescriptorSet ds, 
	const VkDescriptorImageInfo* ii, 
	uint32_t bindIdx)
{
	return VkWriteDescriptorSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, nullptr,
		ds, bindIdx, 0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		ii, nullptr, nullptr
	};
}

RendererCube::RendererCube(VulkanDevice& vkDev, VulkanImage inDepthTexture, const char* textureFile) :
	RendererBase(vkDev, inDepthTexture)
{
	// Resource loading
	texture.CreateCubeTextureImage(vkDev, textureFile);

	texture.image.CreateImageView(
		vkDev.GetDevice(), 
		VK_FORMAT_R32G32B32A32_SFLOAT, 
		VK_IMAGE_ASPECT_COLOR_BIT, 
		VK_IMAGE_VIEW_TYPE_CUBE, 
		6);

	texture.CreateTextureSampler(vkDev.GetDevice());

	std::string vertexShader = AppSettings::ShaderFolder + "cube.vert";
	std::string fragmentShader = AppSettings::ShaderFolder + "cube.frag";

	CreateColorAndDepthRenderPass(vkDev, true, &renderPass_, RenderPassCreateInfo());

	CreateUniformBuffers(vkDev, sizeof(glm::mat4));
	
	CreateColorAndDepthFramebuffers(vkDev, renderPass_, depthTexture_.imageView, swapchainFramebuffers_);
	
	CreateDescriptorPool(vkDev, 1, 0, 1, &descriptorPool_);
	
	CreateDescriptorSet(vkDev);
	
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);
	
	CreateGraphicsPipeline(vkDev,
		renderPass_,
		pipelineLayout_,
		{
			vertexShader.c_str(),
			fragmentShader.c_str(),
		},
		&graphicsPipeline_);
}

RendererCube::~RendererCube()
{
	texture.DestroyVulkanTexture(device_);
}

void RendererCube::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{
	BeginRenderPass(commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void RendererCube::UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const glm::mat4& m)
{
	UploadBufferData(vkDev, uniformBuffers_[currentImage].bufferMemory_, 0, glm::value_ptr(m), sizeof(glm::mat4));
}

bool RendererCube::CreateDescriptorSet(VulkanDevice& vkDev)
{
	const std::array<VkDescriptorSetLayoutBinding, 2> bindings = {
		DescriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT),
		DescriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));

	auto swapChainImageSize = vkDev.GetSwapChainImageSize();

	std::vector<VkDescriptorSetLayout> layouts(swapChainImageSize, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapChainImageSize),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapChainImageSize);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapChainImageSize; i++)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		const VkDescriptorBufferInfo bufferInfo = { uniformBuffers_[i].buffer_, 0, sizeof(glm::mat4) };
		const VkDescriptorImageInfo  imageInfo = { texture.sampler, texture.image.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

		const std::array<VkWriteDescriptorSet, 2> descriptorWrites = {
			BufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			ImageWriteDescriptorSet(ds, &imageInfo,   1)
		};

		vkUpdateDescriptorSets(vkDev.GetDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}

	return true;
}
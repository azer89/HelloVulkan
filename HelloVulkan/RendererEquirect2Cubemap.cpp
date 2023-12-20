#include "RendererEquirect2Cubemap.h"
#include "VulkanUtility.h"
#include "AppSettings.h"

const uint32_t cubemapSideLength = 1024;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

RendererEquirect2Cubemap::RendererEquirect2Cubemap(VulkanDevice& vkDev, const std::string& hdrFile) :
	RendererBase(vkDev, {})
{
	InitializeHDRTexture(vkDev, hdrFile);
	InitializeCubemapTexture(vkDev);
	CreateRenderPass(vkDev);

	CreateDescriptorPool(
		vkDev,
		0, // UBO
		0, // SSBO
		1, // Sampler
		1, // Decsriptor count per swapchain
		&descriptorPool_);

	CreateDescriptorLayout(vkDev);
	CreateDescriptorSet(vkDev);
	CreatePipelineLayout(vkDev.GetDevice(), descriptorSetLayout_, &pipelineLayout_);

	std::string vertFile = AppSettings::ShaderFolder + "fullscreen_triangle.vert";
	std::string fragFile = AppSettings::ShaderFolder + "equirect_2_cubemap.frag";
	CreateGraphicsPipeline(
		vkDev,
		renderPass_,
		pipelineLayout_,
		{
			vertFile.c_str(),
			fragFile.c_str()
		},
		&graphicsPipeline_,
		false // hasVertexBuffer
	);
}

RendererEquirect2Cubemap::~RendererEquirect2Cubemap()
{
	hdrTexture_.DestroyVulkanTexture(device_);
	cubemapTexture_.DestroyVulkanTexture(device_);
}

void RendererEquirect2Cubemap::FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage)
{

}

void RendererEquirect2Cubemap::InitializeCubemapTexture(VulkanDevice& vkDev)
{
	uint32_t mipmapCount = NumMipMap(cubemapSideLength, cubemapSideLength);
	uint32_t layerCount = 6;

	cubemapTexture_.image_.CreateImage(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		cubemapSideLength,
		cubemapSideLength,
		mipmapCount,
		layerCount,
		cubeMapFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	);

	cubemapTexture_.image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		VK_IMAGE_VIEW_TYPE_CUBE,
		layerCount,
		mipmapCount);
}

void RendererEquirect2Cubemap::InitializeHDRTexture(VulkanDevice& vkDev, const std::string& hdrFile)
{
	hdrTexture_.CreateHDRImage(vkDev, hdrFile.c_str());
	hdrTexture_.image_.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	hdrTexture_.CreateTextureSampler(
		vkDev.GetDevice(),
		0.f,
		1.f);
}

void RendererEquirect2Cubemap::CreateRenderPass(VulkanDevice& vkDev)
{
	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<VkAttachmentReference> m_attachmentRefs;

	for (int face = 0; face < 6; ++face)
	{
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription info{};
		info.flags = 0u;
		info.format = cubeMapFormat;
		info.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		info.samples = VK_SAMPLE_COUNT_1_BIT;
		info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		info.finalLayout = finalLayout;
		info.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		info.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		VkAttachmentReference ref{};
		ref.attachment = static_cast<uint32_t>(face);
		ref.layout = finalLayout;

		m_attachments.push_back(info);
		m_attachmentRefs.push_back(ref);
	}

	VkSubpassDescription m_subpass{};
	m_subpass.flags = 0u;
	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	m_subpass.colorAttachmentCount = static_cast<uint32_t>(m_attachmentRefs.size());
	m_subpass.pColorAttachments = m_attachmentRefs.data();

	VkRenderPassCreateInfo m_info{};
	m_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_info.pNext = nullptr;
	m_info.flags = 0u;
	m_info.pSubpasses = &m_subpass;
	m_info.subpassCount = 1u;
	m_info.pAttachments = m_attachments.data();
	m_info.attachmentCount = static_cast<uint32_t>(m_attachments.size());

	VK_CHECK(vkCreateRenderPass(vkDev.GetDevice(), &m_info, nullptr, &renderPass_));
}

bool RendererEquirect2Cubemap::CreateDescriptorLayout(VulkanDevice& vkDev)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	uint32_t bindingIndex = 0;

	// Input HDR
	bindings.emplace_back(
		DescriptorSetLayoutBinding(
			bindingIndex++,
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			VK_SHADER_STAGE_FRAGMENT_BIT)
	);

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.bindingCount = static_cast<uint32_t>(bindings.size()),
		.pBindings = bindings.data()
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vkDev.GetDevice(), &layoutInfo, nullptr, &descriptorSetLayout_));

	return true;
}

bool RendererEquirect2Cubemap::CreateDescriptorSet(VulkanDevice& vkDev)
{
	size_t swapchainLength = vkDev.GetSwapChainImageSize();

	std::vector<VkDescriptorSetLayout> layouts(swapchainLength, descriptorSetLayout_);

	const VkDescriptorSetAllocateInfo allocInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = nullptr,
		.descriptorPool = descriptorPool_,
		.descriptorSetCount = static_cast<uint32_t>(swapchainLength),
		.pSetLayouts = layouts.data()
	};

	descriptorSets_.resize(swapchainLength);

	VK_CHECK(vkAllocateDescriptorSets(vkDev.GetDevice(), &allocInfo, descriptorSets_.data()));

	for (size_t i = 0; i < swapchainLength; i++)
	{
		VkDescriptorSet ds = descriptorSets_[i];

		uint32_t bindIndex = 0;
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		VkDescriptorImageInfo imageInfo =
		{
			hdrTexture_.sampler_,
			hdrTexture_.image_.imageView_,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		descriptorWrites.emplace_back
		(
			ImageWriteDescriptorSet(
				ds,
				&imageInfo,
				bindIndex++)
		);

		vkUpdateDescriptorSets
		(
			vkDev.GetDevice(),
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0,
			nullptr);
	}

	return true;
}
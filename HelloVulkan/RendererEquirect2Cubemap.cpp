#include "RendererEquirect2Cubemap.h"

const uint32_t cubemapSideLength = 1024;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

RendererEquirect2Cubemap::RendererEquirect2Cubemap(VulkanDevice& vkDev, VulkanImage depthTexture) :
	RendererBase(vkDev, depthTexture)
{
	InitializeCubemapTexture(vkDev);
}

RendererEquirect2Cubemap::~RendererEquirect2Cubemap()
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

void RendererEquirect2Cubemap::CreateRenderPass(VulkanDevice& vkDev)
{
	std::vector<VkAttachmentReference> m_attachmentRefs;
	std::vector<VkAttachmentDescription> m_attachments;

	VkSubpassDescription m_subpass{};
	m_subpass.flags = 0u;
	m_subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

	VkRenderPassCreateInfo m_info{};
	m_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	m_info.pNext = nullptr;
	m_info.flags = 0u;
	m_info.pSubpasses = &m_subpass;
	m_info.subpassCount = 1u;

	// TODO
}
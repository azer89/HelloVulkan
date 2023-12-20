#include "RendererEquirect2Cubemap.h"
#include "VulkanUtility.h"

const uint32_t cubemapSideLength = 1024;
const VkFormat cubeMapFormat = VK_FORMAT_R32G32B32A32_SFLOAT;

RendererEquirect2Cubemap::RendererEquirect2Cubemap(VulkanDevice& vkDev) :
	RendererBase(vkDev, {})
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
	uint32_t mipmapCount = NumMipMap(cubemapSideLength, cubemapSideLength);
	uint32_t layerCount = 6;

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
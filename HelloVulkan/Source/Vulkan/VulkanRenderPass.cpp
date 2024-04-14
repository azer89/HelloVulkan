#include "VulkanRenderPass.h"
#include "VulkanCheck.h"

#include <array>
#include <vector>

// Resolve multi-sampled image to single-sampled image
void VulkanRenderPass::CreateResolveMS(
	VulkanContext& ctx,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples
)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 2u;

	const VkAttachmentDescription multisampledAttachment =
	{
		.format = ctx.GetSwapchainImageFormat(),
		.samples = msaaSamples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD, // Just load
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
	};

	constexpr VkAttachmentReference multisampledRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentDescription singleSampledAttachment = {
		.format = ctx.GetSwapchainImageFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
	};

	constexpr VkAttachmentReference singleSampledRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass =
	{
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = 1,
		.pColorAttachments = &multisampledRef,
		.pResolveAttachments = &singleSampledRef,
		.pDepthStencilAttachment = nullptr,
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		.dependencyFlags = 0
	};

	std::array<VkAttachmentDescription, 2> attachments = { multisampledAttachment, singleSampledAttachment };

	const VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1,
		.pDependencies = &dependency,
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateOffScreen(
	VulkanContext& ctx, 
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 1u;

	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;
	const bool clearDepth = renderPassBit_ & RenderPassBit::DepthClear;

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	// for the next onscreen render pass
	const bool colorShaderReadOnly = renderPassBit_ & RenderPassBit::ColorShaderReadOnly;

	const VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = ctx.GetSwapchainImageFormat(),
		.samples = msaaSamples,
		.loadOp =
			clearColor ?
			VK_ATTACHMENT_LOAD_OP_CLEAR :
			VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout =
			clearColor ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout =
			colorShaderReadOnly ?
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = ctx.GetDepthFormat(),
		.samples = msaaSamples,
		.loadOp = clearDepth ?
				VK_ATTACHMENT_LOAD_OP_CLEAR :
				VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearDepth ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	const VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateOnScreen(
	VulkanContext& ctx, 
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 1u;

	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;
	const bool clearDepth = renderPassBit_ & RenderPassBit::DepthClear;
	const bool presentColor = renderPassBit_ & RenderPassBit::ColorPresent;
	
	const VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = ctx.GetSwapchainImageFormat(),
		.samples = msaaSamples,
		.loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearColor ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = presentColor ?
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = ctx.GetDepthFormat(),
		.samples = msaaSamples,
		.loadOp = clearDepth ?
				VK_ATTACHMENT_LOAD_OP_CLEAR :
				VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearDepth ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateDepthOnly(
	VulkanContext& ctx,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 0u;

	const bool clearDepth = renderPassBit_ & RenderPassBit::DepthClear;
	const bool depthShaderReadOnly = renderPassBit_ & RenderPassBit::DepthShaderReadOnly;

	const VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = ctx.GetDepthFormat(),
		.samples = msaaSamples,
		.loadOp = clearDepth ?
				VK_ATTACHMENT_LOAD_OP_CLEAR :
				VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearDepth ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = depthShaderReadOnly ?
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : 
			VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference depthAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 0,
		.pColorAttachments = nullptr,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = &depthAttachmentRef,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1,
		.pAttachments = &depthAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 0,
		.pDependencies = nullptr
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateOnScreenColorOnly(
	VulkanContext& ctx,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 1u;

	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;
	const bool presentColor = renderPassBit_ & RenderPassBit::ColorPresent;

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = ctx.GetSwapchainImageFormat(),
		.samples = msaaSamples,
		.loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearColor ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = presentColor ?
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1u,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateOffScreenColorOnly(
	VulkanContext& ctx,
	VkFormat imageFormat,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 1u;

	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = imageFormat,
		.samples = msaaSamples,
		.loadOp = clearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = clearColor ?
			VK_IMAGE_LAYOUT_UNDEFINED :
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	constexpr VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkSubpassDescription subpass = {
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = nullptr,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = nullptr,
		.pDepthStencilAttachment = nullptr,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = nullptr
	};

	const VkRenderPassCreateInfo renderPassInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.attachmentCount = 1u,
		.pAttachments = &colorAttachment,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &renderPassInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateOffScreenCubemap(
	VulkanContext& ctx,
	VkFormat cubeFormat,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = 6u;

	constexpr VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	std::vector<VkAttachmentDescription> m_attachments;
	std::vector<VkAttachmentReference> m_attachmentRefs;

	for (uint32_t face = 0; face < colorAttachmentCount_; ++face)
	{
		VkAttachmentDescription info =
		{
			.flags = 0u,
			.format = cubeFormat,
			.samples = msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = finalLayout,
		};

		VkAttachmentReference ref =
		{
			.attachment = face,
			.layout = finalLayout
		};

		m_attachments.push_back(info);
		m_attachmentRefs.push_back(ref);
	}

	VkSubpassDescription subpassDesc =
	{
		.flags = 0u,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = static_cast<uint32_t>(m_attachmentRefs.size()),
		.pColorAttachments = m_attachmentRefs.data(),
	};

	const VkRenderPassCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.attachmentCount = static_cast<uint32_t>(m_attachments.size()),
		.pAttachments = m_attachments.data(),
		.subpassCount = 1u,
		.pSubpasses = &subpassDesc,
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &createInfo, nullptr, &handle_));
}

void VulkanRenderPass::CreateOffScreenGBuffer(
	VulkanContext& ctx,
	const std::vector<VkFormat>& formats,
	uint8_t renderPassBit,
	VkSampleCountFlagBits msaaSamples)
{
	device_ = ctx.GetDevice();
	renderPassBit_ = renderPassBit;
	colorAttachmentCount_ = static_cast<uint32_t>(formats.size());

	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;
	const bool clearDepth = renderPassBit_ & RenderPassBit::DepthClear;

	std::vector<VkAttachmentDescription> attachments;
	std::vector<VkAttachmentReference> attachmentRefs;

	for (int i = 0; i < formats.size(); ++i)
	{
		VkAttachmentDescription info =
		{
			.flags = 0u,
			.format = formats[i],
			.samples = msaaSamples,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference ref =
		{
			.attachment = static_cast<uint32_t>(i),
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};

		attachments.push_back(info);
		attachmentRefs.push_back(ref);
	}

	const VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = ctx.GetDepthFormat(),
		.samples = msaaSamples,
		.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = static_cast<uint32_t>(attachments.size()),
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	attachments.push_back(depthAttachment);

	VkSubpassDependency dependency =
	{
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0
	};

	VkSubpassDescription subpass =
	{
		.flags = 0u,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = static_cast<uint32_t>(attachmentRefs.size()),
		.pColorAttachments = attachmentRefs.data(),
		.pDepthStencilAttachment = &depthAttachmentRef,
	};

	const VkRenderPassCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0u,
		.attachmentCount = static_cast<uint32_t>(attachments.size()),
		.pAttachments = attachments.data(),
		.subpassCount = 1u,
		.pSubpasses = &subpass,
		.dependencyCount = 1u,
		.pDependencies = &dependency
	};

	VK_CHECK(vkCreateRenderPass(ctx.GetDevice(), &createInfo, nullptr, &handle_));

	// Cache VkRenderPassBeginInfo
	CreateBeginInfo();
}

void VulkanRenderPass::CreateBeginInfo()
{
	const bool clearColor = renderPassBit_ & RenderPassBit::ColorClear;
	const bool clearDepth = renderPassBit_ & RenderPassBit::DepthClear;

	if (clearColor)
	{
		for (uint32_t i = 0; i < colorAttachmentCount_; ++i)
		{
			clearValues_.push_back({ .color = { 1.0f, 1.0f, 1.0f, 1.0f} });
		}
	}

	if (clearDepth)
	{
		clearValues_.push_back({ .depthStencil = { 1.0f, 0 } });
	}

	beginInfo_ =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = handle_,
		.framebuffer = nullptr, // Has to be set in BeginRenderPass()
		.renderArea = { }, // Has to be set in BeginRenderPass()
		.clearValueCount = static_cast<uint32_t>(clearValues_.size()),
		.pClearValues = clearValues_.size() == 0 ? nullptr : &clearValues_[0]
	};
}

void VulkanRenderPass::BeginRenderPass(
	VulkanContext& ctx,
	VkCommandBuffer commandBuffer, 
	VkFramebuffer framebuffer)
{
	// Make sure beginInfo has been initialized
	// Set framebuffer to beginInfo_
	beginInfo_.framebuffer = framebuffer;
	beginInfo_.renderArea = { 0u, 0u, ctx.GetSwapchainWidth(), ctx.GetSwapchainHeight() };
	
	vkCmdBeginRenderPass(commandBuffer, &beginInfo_, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::BeginRenderPass(
	VulkanContext& ctx,
	VkCommandBuffer commandBuffer,
	VkFramebuffer framebuffer,
	uint32_t width,
	uint32_t height)
{
	beginInfo_.framebuffer = framebuffer;
	beginInfo_.renderArea = { 0u, 0u, width, height };

	vkCmdBeginRenderPass(commandBuffer, &beginInfo_, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::BeginCubemapRenderPass(
	VkCommandBuffer commandBuffer,
	VkFramebuffer framebuffer,
	uint32_t cubeSideLength)
{
	// We don't cache buffer info because this is for one-time rendering
	const std::vector<VkClearValue> clearValues(6u, { 0.0f, 0.0f, 1.0f, 1.0f });
	const VkRenderPassBeginInfo info =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = nullptr,
		.renderPass = handle_,
		.framebuffer = framebuffer,
		.renderArea = { 0u, 0u, cubeSideLength, cubeSideLength },
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = clearValues.data(),
	};

	vkCmdBeginRenderPass(commandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::Destroy()
{
	if (handle_)
	{
		vkDestroyRenderPass(device_, handle_, nullptr);
		handle_ = nullptr;
	}
}

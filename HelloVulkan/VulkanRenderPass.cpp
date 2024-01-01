#include "VulkanRenderPass.h"
#include "VulkanUtility.h"

#include <array>
#include <vector>

VulkanRenderPass::VulkanRenderPass()
{
}

VulkanRenderPass::~VulkanRenderPass()
{
}

void VulkanRenderPass::CreateOffScreenRenderPass(VulkanDevice& vkDev, uint8_t renderPassBit)
{
	renderPassBit_ = renderPassBit;

	bool clearColor = renderPassBit_ & RenderPassBit::OffScreenColorClear;

	// Transition color attachment to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
	// for the next onscreen render pass
	bool colorShaderReadOnly = renderPassBit_ & RenderPassBit::OffScreenColorShaderReadOnly;

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = vkDev.GetSwaphchainImageFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
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

	const VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = vkDev.FindDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	const VkAttachmentReference depthAttachmentRef = {
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	std::vector<VkSubpassDependency> dependencies = {
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		},
		{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		}
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
		.dependencyCount = static_cast<uint32_t>(dependencies.size()),
		.pDependencies = dependencies.data()
	};

	VK_CHECK(vkCreateRenderPass(vkDev.GetDevice(), &renderPassInfo, nullptr, &handle_));
}

void VulkanRenderPass::CreateOnScreenRenderPass(VulkanDevice& vkDev, uint8_t renderPassBit)
{
	renderPassBit_ = renderPassBit;

	bool clearColor = renderPassBit_ & RenderPassBit::OnScreenColorClear;
	bool presentColor = renderPassBit_ & RenderPassBit::OnScreenColorPresent;
	bool clearDepth = renderPassBit_ & RenderPassBit::OnScreenDepthClear;

	VkAttachmentDescription colorAttachment = {
		.flags = 0,
		.format = vkDev.GetSwaphchainImageFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
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

	const VkAttachmentReference colorAttachmentRef = {
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment = {
		.flags = 0,
		.format = vkDev.FindDepthFormat(),
		.samples = VK_SAMPLE_COUNT_1_BIT,
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

	const VkAttachmentReference depthAttachmentRef = {
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

	VK_CHECK(vkCreateRenderPass(vkDev.GetDevice(), &renderPassInfo, nullptr, &handle_));
}

VkRenderPassBeginInfo VulkanRenderPass::CreateBeginInfo(VulkanDevice& vkDev)
{
	bool clearColor = 
		renderPassBit_ & RenderPassBit::OnScreenColorClear || 
		renderPassBit_ & RenderPassBit::OffScreenColorClear;
	bool clearDepth = 
		renderPassBit_ & RenderPassBit::OnScreenDepthClear ||
		renderPassBit_ & RenderPassBit::OffScreenColorClear;

	std::vector<VkClearValue> clearValues;
	if (clearColor)
	{
		clearValues.push_back({.color = { 1.0f, 1.0f, 1.0f, 1.0f}});
	}
	if (clearDepth)
	{
		clearValues.push_back({.depthStencil = { 1.0f, 0 }});
	}

	const VkRect2D screenRect = {
		.offset = { 0, 0 },
		.extent = {.width = vkDev.GetFrameBufferWidth(), .height = vkDev.GetFrameBufferHeight()}
	};

	return 
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = handle_,
		.framebuffer = nullptr, // Set this
		.renderArea = screenRect,
		.clearValueCount = static_cast<uint32_t>(clearValues.size()),
		.pClearValues = &clearValues[0]
	};
}

void VulkanRenderPass::BeginRenderPass(
	VulkanDevice& vkDev, 
	VkCommandBuffer commandBuffer, 
	VkFramebuffer framebuffer)
{
	// TODO Cache beginInfo
	VkRenderPassBeginInfo beginInfo = CreateBeginInfo(vkDev);
	beginInfo.framebuffer = framebuffer;
	vkCmdBeginRenderPass(commandBuffer, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void VulkanRenderPass::Destroy(VkDevice device)
{
	vkDestroyRenderPass(device, handle_, nullptr);
}

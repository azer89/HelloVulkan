#ifndef PIPELINE_CREATE_INFO
#define PIPELINE_CREATE_INFO

#include "VulkanDevice.h"

// Default pipeline create info
struct PipelineCreateInfo
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo inputAssembly;
	VkViewport viewport;
	VkRect2D scissor;
	VkPipelineViewportStateCreateInfo viewportState;
	VkPipelineRasterizationStateCreateInfo rasterizer;
	VkPipelineMultisampleStateCreateInfo multisampling;
	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	VkPipelineColorBlendStateCreateInfo colorBlending;
	VkPipelineDepthStencilStateCreateInfo depthStencil;
	VkPipelineDynamicStateCreateInfo dynamicState;
	VkPipelineTessellationStateCreateInfo tessellationState;

	PipelineCreateInfo(VulkanDevice& vkDev) :
		vertexInputInfo({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.vertexBindingDescriptionCount = 0,
			.pVertexBindingDescriptions = nullptr,
			.vertexAttributeDescriptionCount = 0,
			.pVertexAttributeDescriptions = nullptr
		}),
		inputAssembly({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		}),
		viewport({
			.x = 0.0f,
			.y = 0.0f,
			// Note change these when doing offscreen rendering
			.width = static_cast<float>(vkDev.GetFrameBufferWidth()),
			.height = static_cast<float>(vkDev.GetFrameBufferHeight()),
			.minDepth = 0.0f,
			.maxDepth = 1.0f
		}),
		scissor({
			.offset = { 0, 0 },
			// Note change these when doing offscreen rendering
			.extent = { vkDev.GetFrameBufferWidth(), vkDev.GetFrameBufferHeight()}
		}),
		viewportState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.pViewports = &viewport,
			.scissorCount = 1,
			.pScissors = &scissor
		}),
		rasterizer({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = VK_POLYGON_MODE_FILL,
			.cullMode = VK_CULL_MODE_NONE,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.depthBiasConstantFactor = 1.f,
			.depthBiasClamp = 0.f,
			.depthBiasSlopeFactor = 1.f,
			.lineWidth = 1.f
		}),
		multisampling({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0u,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE,
			.minSampleShading = 0.f,
			.pSampleMask = nullptr,
			.alphaToCoverageEnable = VK_FALSE,
			.alphaToOneEnable = VK_FALSE
		}),
		colorBlendAttachment({
			.blendEnable = VK_TRUE,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		}),
		colorBlending({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = 1,
			.pAttachments = &colorBlendAttachment,
			.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
		}),
		depthStencil({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL, // Needed for skybox rendering
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
		}),
		// Change below if you want window resizing
		dynamicState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.dynamicStateCount = 0u,
			.pDynamicStates = nullptr
		}),
		tessellationState({
			.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
			.pNext = nullptr,
			.flags = 0,
			.patchControlPoints = 0
		})
	{
	}
};

#endif
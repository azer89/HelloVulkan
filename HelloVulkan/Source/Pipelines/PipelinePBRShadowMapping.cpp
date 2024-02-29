#include "PipelinePBRShadowMapping.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 5;
constexpr uint32_t ENV_TEXTURE_COUNT = 4; // Specular, diffuse, BRDF LUT, and shadow map

PipelinePBRShadowMapping::PipelinePBRShadowMapping(
	VulkanContext& ctx,
	Scene* scene,
	Lights* lights,
	IBLResources* iblResources,
	VulkanImage* shadowMap,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,

			// If you use bindless, make sure this is false
			.vertexBufferBind_ = false,
		}
	),
	scene_(scene),
	lights_(lights),
	iblResources_(iblResources),
	shadowMap_(shadowMap)
{
	// UBOs
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	CreateMultipleUniformBuffers(ctx, shadowMapConfigUBOBuffers_, sizeof(ShadowMapUBO), AppConfig::FrameOverlapCount);

	CreateIndirectBuffers(ctx, scene_, indirectBuffers_);

	// Note that this pipeline is offscreen rendering
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);

	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			offscreenColorImage,
			depthImage
		}, 
		IsOffscreen());

	CreateDescriptor(ctx);

	// Push constants
	std::vector<VkPushConstantRange> ranges =
	{{
		.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		.offset = 0u,
		.size = sizeof(PushConstantPBR),
	}};
	
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, ranges);

	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "ShadowMapping//Scene.vert",
			AppConfig::ShaderFolder + "ShadowMapping//Scene.frag"
		},
		&pipeline_
	);
}

PipelinePBRShadowMapping::~PipelinePBRShadowMapping()
{
	for (auto uboBuffer : shadowMapConfigUBOBuffers_)
	{
		uboBuffer.Destroy();
	}

	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}
}

void PipelinePBRShadowMapping::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantPBR), &pc_);

	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0u,
		1u,
		&(descriptorSets_[frameIndex]),
		0u,
		nullptr);

	vkCmdDrawIndirect(
		commandBuffer,
		indirectBuffers_[frameIndex].buffer_,
		0, // offset
		scene_->GetMeshCount(),
		sizeof(VkDrawIndirectCommand));
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRShadowMapping::CreateDescriptor(VulkanContext& ctx)
{
	/*11*/ std::vector<VkDescriptorImageInfo> imageInfos = scene_->GetImageInfos();

	const uint32_t textureCount = static_cast<uint32_t>(imageInfos.size());
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	// Pool
	descriptor_.CreatePool(
		ctx,
		{
			.uboCount_ = UBO_COUNT,
			.ssboCount_ = SSBO_COUNT,
			.samplerCount_ = (textureCount + ENV_TEXTURE_COUNT),
			.frameCount_ = frameCount,
			.setCountPerFrame_ = 1,
		});

	// Layout
	descriptor_.CreateLayout(ctx,
	{
		{
			.type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 2u
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.shaderFlags_ = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = 5u
		},
		{
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.bindingCount_ = ENV_TEXTURE_COUNT
		},
		{
			// NOTE Unbounded array should be the last
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.shaderFlags_ = VK_SHADER_STAGE_FRAGMENT_BIT,
			.descriptorCount_ = textureCount,
			.bindingCount_ = 1u
		}
	});

	// Set
	/*3*/ VkDescriptorBufferInfo vertexBufferInfo = scene_->vertexBuffer_.GetBufferInfo();
	/*4*/ VkDescriptorBufferInfo indexBufferInfo = scene_->indexBuffer_.GetBufferInfo();
	/*5*/ VkDescriptorBufferInfo meshBufferInfo = scene_->meshDataBuffer_.GetBufferInfo();
	/*6*/ VkDescriptorBufferInfo lightBufferInfo = lights_->GetBufferInfo();
	/*7*/ VkDescriptorImageInfo specularImageInfo = iblResources_->specularCubemap_.GetDescriptorImageInfo();
	/*8*/ VkDescriptorImageInfo diffuseImageInfo = iblResources_->diffuseCubemap_.GetDescriptorImageInfo();
	/*9*/ VkDescriptorImageInfo lutImageInfo = iblResources_->brdfLut_.GetDescriptorImageInfo();
	/*10*/ VkDescriptorImageInfo shadowImageInfo = shadowMap_->GetDescriptorImageInfo();

	descriptorSets_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		/*0*/ VkDescriptorBufferInfo camBufferInfo = cameraUBOBuffers_[i].GetBufferInfo();
		/*1*/ VkDescriptorBufferInfo shadowUBOBufferInfo = shadowMapConfigUBOBuffers_[i].GetBufferInfo();
		/*2*/ VkDescriptorBufferInfo modelBufferInfo = scene_->modelUBOBuffers_[i].GetBufferInfo();
		
		std::vector<DescriptorSetWrite> writes;
		/*0*/ writes.push_back({ .bufferInfoPtr_ = &camBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*1*/ writes.push_back({ .bufferInfoPtr_ = &shadowUBOBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER });
		/*2*/ writes.push_back({ .bufferInfoPtr_ = &modelBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*3*/ writes.push_back({ .bufferInfoPtr_ = &vertexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*4*/ writes.push_back({ .bufferInfoPtr_ = &indexBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*5*/ writes.push_back({ .bufferInfoPtr_ = &meshBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*6*/ writes.push_back({ .bufferInfoPtr_ = &lightBufferInfo, .type_ = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER });
		/*7*/ writes.push_back({ .imageInfoPtr_ = &specularImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*8*/ writes.push_back({ .imageInfoPtr_ = &diffuseImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*9*/ writes.push_back({ .imageInfoPtr_ = &lutImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*10*/ writes.push_back({ .imageInfoPtr_ = &shadowImageInfo, .type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
		/*11*/ writes.push_back({
			.imageInfoPtr_ = imageInfos.data(),
			.descriptorCount_ = textureCount,
			.type_ = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });

		descriptor_.CreateSet(ctx, writes, &(descriptorSets_[i]));
	}
}
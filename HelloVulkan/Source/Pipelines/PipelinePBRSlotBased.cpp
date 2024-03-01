#include "PipelinePBRSlotBased.h"
#include "VulkanUtility.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 1;
constexpr uint32_t PBR_MESH_TEXTURE_COUNT = 6;
constexpr uint32_t PBR_ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRSlotBased::PipelinePBRSlotBased(
	VulkanContext& ctx,
	const std::vector<Model*>& models,
	Lights* lights,
	IBLResources* iblResources,
	VulkanImage* depthImage,
	VulkanImage* offscreenColorImage,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = offscreenColorImage->multisampleCount_,
			.vertexBufferBind_ = true,
		}
	),
	lights_(lights),
	iblResources_(iblResources),
	models_(models)
{
	// Per frame UBO
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);

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
	const std::vector<VkPushConstantRange> ranges =
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
			AppConfig::ShaderFolder + "SlotBased//Mesh.vert",
			AppConfig::ShaderFolder + "SlotBased//Mesh.frag"
		},
		&pipeline_
	);
}

PipelinePBRSlotBased::~PipelinePBRSlotBased()
{
}

void PipelinePBRSlotBased::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantPBR), &pc_);

	size_t meshIndex = 0;
	for (Model* model : models_)
	{
		for (Mesh& mesh : model->meshes_)
		{
			vkCmdBindDescriptorSets(
				commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout_,
				0,
				1,
				&(descriptorSets_[frameIndex][meshIndex++]),
				0,
				nullptr);

			// Bind vertex buffer
			VkBuffer buffers[] = { mesh.vertexBuffer_.buffer_ };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

			// Bind index buffer
			vkCmdBindIndexBuffer(commandBuffer, mesh.indexBuffer_.buffer_, 0, VK_INDEX_TYPE_UINT32);

			// Draw
			vkCmdDrawIndexed(commandBuffer, mesh.GetNumIndices(), 1, 0, 0, 0);
		}
	}
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRSlotBased::CreateDescriptor(VulkanContext& ctx)
{
	uint32_t numMeshes = 0u;
	for (Model* model : models_)
	{
		numMeshes += model->NumMeshes();
	}

	DescriptorBuildInfo buildInfo;
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	buildInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	buildInfo.AddBuffer(lights_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	for (size_t i = 0; i < PBR_MESH_TEXTURE_COUNT; ++i)
	{
		buildInfo.AddImage(nullptr);
	}
	buildInfo.AddImage(&(iblResources_->specularCubemap_));
	buildInfo.AddImage(&(iblResources_->diffuseCubemap_));
	buildInfo.AddImage(&(iblResources_->brdfLut_));

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, buildInfo, AppConfig::FrameOverlapCount, numMeshes);

	// Sets
	constexpr size_t bindingOffset = static_cast<size_t>(UBO_COUNT + SSBO_COUNT);
	descriptorSets_.resize(AppConfig::FrameOverlapCount);
	for (size_t i = 0; i < AppConfig::FrameOverlapCount; i++)
	{
		size_t meshIndex = 0;
		descriptorSets_[i].resize(numMeshes);
		buildInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		for (Model* model : models_)
		{
			buildInfo.UpdateBuffer(&(model->modelBuffers_[i]), 1);
			for (Mesh& mesh : model->meshes_)
			{
				for (const auto& elem : mesh.textureIndices_)
				{
					// Should be ordered based on elem.first
					size_t typeIndex = static_cast<size_t>(elem.first) - 1;
					int textureIndex = elem.second;
					VulkanImage* texture = model->GetTexture(textureIndex);
					buildInfo.UpdateImage(texture, bindingOffset + typeIndex);
				}
				descriptor_.CreateSet(ctx, buildInfo, &(descriptorSets_[i][meshIndex]));
				meshIndex++;
			}
		}
	}
}
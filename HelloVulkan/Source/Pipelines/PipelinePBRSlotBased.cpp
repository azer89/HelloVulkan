#include "PipelinePBRSlotBased.h"
#include "Model.h"
#include "ResourcesShared.h"
#include "ResourcesLight.h"
#include "ResourcesIBL.h"
#include "Configs.h"

#include <vector>

// Constants
constexpr uint32_t UBO_COUNT = 2;
constexpr uint32_t SSBO_COUNT = 1;
constexpr uint32_t PBR_TEXTURE_COUNT = 6;
constexpr uint32_t ENV_TEXTURE_COUNT = 3; // Specular, diffuse, and BRDF LUT

PipelinePBRSlotBased::PipelinePBRSlotBased(
	VulkanContext& ctx,
	const std::vector<Model*>& models,
	ResourcesLight* resourcesLight,
	ResourcesIBL* resourcesIBL,
	ResourcesShared* resourcesShared,
	uint8_t renderBit) :
	PipelineBase(ctx, 
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resourcesShared->multiSampledColorImage_.multisampleCount_,
			.vertexBufferBind_ = true,
		}
	),
	resourcesLight_(resourcesLight),
	resourcesIBL_(resourcesIBL),
	models_(models)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx, 
		renderPass_.GetHandle(), 
		{
			&(resourcesShared->multiSampledColorImage_),
			&(resourcesShared->depthImage_)
		}, 
		IsOffscreen());
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_, sizeof(PushConstPBR), VK_SHADER_STAGE_FRAGMENT_BIT);
	CreateGraphicsPipeline(
		ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "SlotBased/Mesh.vert",
			AppConfig::ShaderFolder + "SlotBased/Mesh.frag"
		},
		&pipeline_
	);
}

PipelinePBRSlotBased::~PipelinePBRSlotBased()
{
}

void PipelinePBRSlotBased::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "PBR_Slot_Based", tracy::Color::Orange4);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());

	BindPipeline(ctx, commandBuffer);

	vkCmdPushConstants(
		commandBuffer,
		pipelineLayout_,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstPBR), &pc_);

	ctx.InsertDebugLabel(commandBuffer, "PipelinePBRSlotBased", 0xffff9999);

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
			vkCmdDrawIndexed(commandBuffer, mesh.GetIndexCount(), 1, 0, 0, 0);
		}
	}
	
	vkCmdEndRenderPass(commandBuffer);
}

void PipelinePBRSlotBased::CreateDescriptor(VulkanContext& ctx)
{
	uint32_t meshCount = 0u;
	for (Model* model : models_)
	{
		meshCount += model->GetMeshCount();
	}

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	dsInfo.AddBuffer(resourcesLight_->GetVulkanBufferPtr(), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	for (size_t i = 0; i < PBR_TEXTURE_COUNT; ++i)
	{
		dsInfo.AddImage(nullptr);
	}
	dsInfo.AddImage(&(resourcesIBL_->specularCubemap_));
	dsInfo.AddImage(&(resourcesIBL_->diffuseCubemap_));
	dsInfo.AddImage(&(resourcesIBL_->brdfLut_));

	// Pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, AppConfig::FrameCount, meshCount);

	// Sets
	constexpr uint32_t bindingOffset = static_cast<size_t>(UBO_COUNT + SSBO_COUNT);
	descriptorSets_.resize(AppConfig::FrameCount);
	for (size_t i = 0; i < AppConfig::FrameCount; i++)
	{
		uint32_t meshIndex = 0;
		descriptorSets_[i].resize(meshCount);
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		for (Model* model : models_)
		{
			dsInfo.UpdateBuffer(&(model->modelBuffers_[i]), 1);
			for (Mesh& mesh : model->meshes_)
			{
				for (const auto& elem : mesh.textureIndices_)
				{
					// Should be ordered based on elem.first
					const uint32_t typeIndex = static_cast<uint32_t>(elem.first) - 1;
					const uint32_t textureIndex = static_cast<uint32_t>(elem.second);
					const VulkanImage* texture = model->GetTexture(textureIndex);
					dsInfo.UpdateImage(texture, bindingOffset + typeIndex);
				}
				descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i][meshIndex]));
				meshIndex++;
			}
		}
	}
}
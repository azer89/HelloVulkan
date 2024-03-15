#include "PipelineLine.h"
#include "ResourcesShared.h"
#include "Scene.h"

#include "glm/ext.hpp"

const glm::vec4 BOX_COLOR = glm::vec4(0.988, 0.4, 0.212, 1.0);

PipelineLine::PipelineLine(
	VulkanContext& ctx,
	ResourcesShared* resShared,
	Scene* scene,
	uint8_t renderBit) :
	PipelineBase(ctx,
		{
			.type_ = PipelineType::GraphicsOffScreen,
			.msaaSamples_ = resShared->multiSampledColorImage_.multisampleCount_,
			.topology_ = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,

			.vertexBufferBind_ = false,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	scene_(scene),
	shouldRender_(false)
{
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit, config_.msaaSamples_);
	framebuffer_.CreateResizeable(
		ctx,
		renderPass_.GetHandle(),
		{
			&(resShared->multiSampledColorImage_),
			&(resShared->depthImage_)
		},
		IsOffscreen()
	);

	ProcessScene(ctx);
	CreateDescriptor(ctx);
	CreatePipelineLayout(ctx, descriptor_.layout_, &pipelineLayout_);
	CreateGraphicsPipeline(ctx,
		renderPass_.GetHandle(),
		pipelineLayout_,
		{
			AppConfig::ShaderFolder + "Line.vert",
			AppConfig::ShaderFolder + "Line.frag",
		},
		&pipeline_
	);
}

PipelineLine::~PipelineLine()
{
	lineBuffer_.Destroy();
}

void PipelineLine::FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer)
{
	if (!shouldRender_)
	{
		return;
	}

	TracyVkZoneC(ctx.GetTracyContext(), commandBuffer, "Lines", tracy::Color::PaleVioletRed);

	const uint32_t frameIndex = ctx.GetFrameIndex();
	renderPass_.BeginRenderPass(ctx, commandBuffer, framebuffer_.GetFramebuffer());
	BindPipeline(ctx, commandBuffer);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[frameIndex],
		0,
		nullptr);

	vkCmdDraw(commandBuffer, static_cast<uint32_t>(lineDataArray_.size()), 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineLine::ProcessScene(VulkanContext& ctx)
{
	// Build bounding boxes
	for (size_t i = 0; i < scene_->originalBoundingBoxes_.size(); ++i)
	{
		const MeshData& mData = scene_->meshDataArray_[i];
		const glm::mat4& mat = scene_->modelUBOs_[mData.modelIndex].model;
		const BoundingBox& box = scene_->originalBoundingBoxes_[i];
		AddBox(
			mat * glm::translate(glm::mat4(1.f), 0.5f * (box.min_ + box.max_)), // mat
			0.5f * glm::vec3(box.max_ - box.min_), // size
			BOX_COLOR);
	}

	// Create buffer
	VkDeviceSize bufferSize = lineDataArray_.size() * sizeof(PointColor);
	lineBuffer_.CreateBuffer(
		ctx,
		bufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	lineBuffer_.UploadBufferData(ctx, lineDataArray_.data(), bufferSize);
}

void PipelineLine::AddBox(const glm::mat4& mat, const glm::vec3& size, const glm::vec4& color)
{
	std::array<glm::vec3, 8> pts = {
		glm::vec3(+size.x, +size.y, +size.z),
		glm::vec3(+size.x, +size.y, -size.z),
		glm::vec3(+size.x, -size.y, +size.z),
		glm::vec3(+size.x, -size.y, -size.z),
		glm::vec3(-size.x, +size.y, +size.z),
		glm::vec3(-size.x, +size.y, -size.z),
		glm::vec3(-size.x, -size.y, +size.z),
		glm::vec3(-size.x, -size.y, -size.z),
	};

	for (auto& p : pts)
	{
		p = glm::vec3(mat * glm::vec4(p, 1.f));
	}

	AddLine(pts[0], pts[1], color);
	AddLine(pts[2], pts[3], color);
	AddLine(pts[4], pts[5], color);
	AddLine(pts[6], pts[7], color);

	AddLine(pts[0], pts[2], color);
	AddLine(pts[1], pts[3], color);
	AddLine(pts[4], pts[6], color);
	AddLine(pts[5], pts[7], color);

	AddLine(pts[0], pts[4], color);
	AddLine(pts[1], pts[5], color);
	AddLine(pts[2], pts[6], color);
	AddLine(pts[3], pts[7], color);
}

void PipelineLine::AddLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color)
{
	lineDataArray_.push_back({ .position = glm::vec4(p1, 1.0), .color = color });
	lineDataArray_.push_back({ .position = glm::vec4(p2, 1.0), .color = color });
}

void PipelineLine::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameOverlapCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 0
	dsInfo.AddBuffer(&lineBuffer_, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}
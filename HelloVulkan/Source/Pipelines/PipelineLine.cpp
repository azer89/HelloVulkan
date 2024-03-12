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
			.topology_ = VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
			.vertexBufferBind_ = false,
			.depthTest_ = true,
			.depthWrite_ = false // Do not write to depth image
		}),
	scene_(scene),
	shouldRender_(false)
{
	CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameOverlapCount);
	renderPass_.CreateOffScreenRenderPass(ctx, renderBit);
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
}

void PipelineLine::ProcessScene(VulkanContext& ctx)
{
	// Build bounding boxes
	for (size_t i = 0; i < scene_->boundingBoxes_.size(); ++i)
	{
		const MeshData& mData = scene_->meshDataArray_[i];
		const glm::mat4& mat = scene_->modelUBOs_[mData.modelIndex].model;
		const BoundingBox& box = scene_->boundingBoxes_[i];
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

	for (size_t i = 0; i < frameCount; ++i)
	{

	}
}
#include "PipelineLine.h"
#include "ResourcesShared.h"
#include "Scene.h"

#include "glm/ext.hpp"

const glm::vec4 LINE_COLOR = glm::vec4(0.988, 0.4, 0.212, 1.0);
constexpr float LINE_WIDTH = 4.0f;

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
			.depthWrite_ = false, // Do not write to depth image
			.lineWidth_ = LINE_WIDTH
		}),
	scene_(scene),
	shouldRender_(false)
{
	VulkanBuffer::CreateMultipleUniformBuffers(ctx, cameraUBOBuffers_, sizeof(CameraUBO), AppConfig::FrameCount);
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
	// Initialize
	//ProcessScene();
	InitFrustumLines();
	CreateBuffers(ctx);
	/*for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		UploadLinesToBuffer(ctx, i);
	}*/
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
	for (auto& buffer : lineBuffers_)
	{
		buffer.Destroy();
	}
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
	vkCmdSetLineWidth(commandBuffer, LINE_WIDTH);
	vkCmdBindDescriptorSets(
		commandBuffer,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		pipelineLayout_,
		0,
		1,
		&descriptorSets_[frameIndex],
		0,
		nullptr);

	ctx.InsertDebugLabel(commandBuffer, "PipelineLine", 0xffff9999);

	vkCmdDraw(commandBuffer, 
		static_cast<uint32_t>(lineDataArray_.size() + frustumDataArray_.size()), 
		1, 
		0, 
		0);

	vkCmdEndRenderPass(commandBuffer);
}

void PipelineLine::SetFrustum(VulkanContext& ctx, CameraUBO& camUBO)
{
	const glm::vec3 corners[] = {
		glm::vec3(+1, -1, -1), glm::vec3(+1, -1, +1),
		glm::vec3(+1, +1, -1), glm::vec3(+1, +1, +1),
		glm::vec3(-1, +1, -1), glm::vec3(-1, +1, +1),
		glm::vec3(-1, -1, -1), glm::vec3(-1, -1, +1)
	};
	glm::vec3 points[8];

	for (int i = 0; i < 8; i++)
	{
		glm::vec4 q = glm::inverse(camUBO.view) * glm::inverse(camUBO.projection) * glm::vec4(corners[i], 1.0f);
		points[i] = glm::vec3(q.x / q.w, q.y / q.w, q.z / q.w);
	}

	int counter = 0;
	for (int i = 0; i < 4; i++)
	{
		UpdateFrustumLine(counter, points[i * 2 + 0], points[i * 2 + 1], LINE_COLOR);
		counter += 2;

		for (int k = 0; k < 2; k++)
		{
			UpdateFrustumLine(counter, points[k + i * 2], points[k + ((i + 1) % 4) * 2], LINE_COLOR);
			counter += 2;
		}
	}

	const uint32_t frameIndex = ctx.GetFrameIndex();
	UploadFrustumLinesToBuffer(ctx, frameIndex);
}

void PipelineLine::ProcessScene()
{
	// Build bounding boxes
	/*for (size_t i = 0; i < scene_->instanceDataArray_.size(); ++i)
	{
		const MeshData& mData = scene_->instanceDataArray_[i].meshData;
		const glm::mat4& mat = scene_->modelSSBOs_[mData.modelMatrixIndex_].model;
		const BoundingBox& box = scene_->instanceDataArray_[i].originalBoundingBox;
		AddBox(
			mat * glm::translate(glm::mat4(1.f), 0.5f * glm::vec3(box.min_ + box.max_)), // mat
			0.5f * glm::vec3(box.max_ - box.min_), // size
			LINE_COLOR);
	}*/
	for (size_t i = 0; i < scene_->instanceDataArray_.size(); ++i)
	{
		MeshData& mData = scene_->instanceDataArray_[i].meshData_;
		glm::mat4& mat = scene_->modelSSBOs_[mData.modelMatrixIndex_].model;
		BoundingBox& box = scene_->instanceDataArray_[i].originalBoundingBox_;
		AddBox(
			mat * glm::translate(glm::mat4(1.f), 0.5f * glm::vec3(box.min_ + box.max_)), // mat
			0.5f * glm::vec3(box.max_ - box.min_), // size
			LINE_COLOR);
	}
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

void PipelineLine::InitFrustumLines()
{
	frustumDataArray_.resize(24);
}

void PipelineLine::UpdateFrustumLine(int index, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color)
{
	frustumDataArray_[index] = { .position = glm::vec4(p1, 1.0), .color = color };
	frustumDataArray_[index + 1] = { .position = glm::vec4(p2, 1.0), .color = color };
}

void PipelineLine::UploadLinesToBuffer(VulkanContext& ctx, uint32_t frameIndex)
{
	// This is the first data so need to offset
	lineBuffers_[frameIndex].UploadBufferData(ctx, lineDataArray_.data(), lineDataArray_.size() * sizeof(PointColor));
}

void PipelineLine::UploadFrustumLinesToBuffer(VulkanContext& ctx, uint32_t frameIndex)
{
	VkDeviceSize offset = lineDataArray_.size() * sizeof(PointColor);
	lineBuffers_[frameIndex].UploadOffsetBufferData(
		ctx, 
		frustumDataArray_.data(),
		offset,
		frustumDataArray_.size() * sizeof(PointColor));
}

void PipelineLine::CreateBuffers(VulkanContext& ctx)
{
	VkDeviceSize bufferSize = (lineDataArray_.size() + frustumDataArray_.size()) * sizeof(PointColor);
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		lineBuffers_[i].CreateBuffer(
			ctx,
			bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
	}
}

void PipelineLine::CreateDescriptor(VulkanContext& ctx)
{
	constexpr uint32_t frameCount = AppConfig::FrameCount;

	VulkanDescriptorInfo dsInfo;
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 0
	dsInfo.AddBuffer(nullptr, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT); // 1

	// Create pool and layout
	descriptor_.CreatePoolAndLayout(ctx, dsInfo, frameCount, 1u);

	for (size_t i = 0; i < frameCount; ++i)
	{
		dsInfo.UpdateBuffer(&(cameraUBOBuffers_[i]), 0);
		dsInfo.UpdateBuffer(&(lineBuffers_[i]), 1);
		descriptor_.CreateSet(ctx, dsInfo, &(descriptorSets_[i]));
	}
}
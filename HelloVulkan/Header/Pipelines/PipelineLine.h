#ifndef PIPELINE_LINE
#define PIPELINE_LINE

#include "PipelineBase.h"
#include "VulkanBuffer.h"
#include "BoundingBox.h"
#include "Scene.h"
#include "ResourcesShared.h"
#include "Configs.h"

#include <array>
#include <vector>

struct PointColor
{
	glm::vec4 position;
	glm::vec4 color;
};

class PipelineLine final : public PipelineBase
{
public:
	PipelineLine(
		VulkanContext& ctx,
		ResourcesShared* resShared,
		Scene* scene,
		uint8_t renderBit = 0u);
	~PipelineLine();

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;
	void SetFrustum(VulkanContext& ctx, CameraUBO& camUBO);

	void UpdateFromIUData(VulkanContext& ctx, UIData& uiData) override
	{
		shouldRender_ = uiData.renderDebug_;
	}

private:
	void CreateDescriptor(VulkanContext& ctx);
	void CreateBuffers(VulkanContext& ctx);

	void ProcessScene();
	void AddBox(const glm::mat4& mat, const glm::vec3& size, const glm::vec4& color);
	void AddLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);
	void UploadLinesToBuffer(VulkanContext& ctx, uint32_t frameIndex);

	void InitFrustumLines();
	void UpdateFrustumLine(int index, const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);
	void UploadFrustumLinesToBuffer(VulkanContext& ctx, uint32_t frameIndex);

private:
	bool shouldRender_;

	// Bounding box rendering
	Scene* scene_;
	std::vector<PointColor> lineDataArray_;
	std::array<VulkanBuffer, AppConfig::FrameCount> lineBuffers_;
	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

	// Camera frustum rendering
	std::vector<PointColor> frustumDataArray_;
};

#endif
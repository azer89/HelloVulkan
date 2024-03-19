#ifndef PIPELINE_LINE
#define PIPELINE_LINE

#include "PipelineBase.h"
#include "VulkanBuffer.h"
#include "BoundingBox.h"
#include "Configs.h"

#include <array>
#include <vector>

class Scene;
struct ResourcesShared;

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
	void ShouldRender(bool shouldRender) { shouldRender_ = shouldRender; };
	void SetFrustum(VulkanContext& ctx, const CameraUBO& camUBO);

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
	size_t frustumPointOffset_;
	size_t frustumPointCount_;
	std::vector<PointColor> frustumDataArray_;
};

#endif
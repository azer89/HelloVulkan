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

private:
	void CreateDescriptor(VulkanContext& ctx);
	void ProcessScene(VulkanContext& ctx);
	void AddBox(const glm::mat4& mat, const glm::vec3& size, const glm::vec4& color);
	void AddLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec4& color);

private:
	bool shouldRender_;
	Scene* scene_;
	std::vector<PointColor> lineDataArray_;
	VulkanBuffer lineBuffer_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
};

#endif
#ifndef PIPELINE_LINE
#define PIPELINE_LINE

#include "PipelineBase.h"
#include "VulkanBuffer.h"
#include "Configs.h"

#include <array>
#include <vector>

class Scene;
struct ResourcesShared;

struct LineData
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
	void CreateLines(VulkanContext& ctx);

private:
	bool shouldRender_;
	Scene* scene_;
	std::vector<LineData> lineDataArray_;
	VulkanBuffer lineBuffer_;
	std::array<VkDescriptorSet, AppConfig::FrameOverlapCount> descriptorSets_;
};

#endif
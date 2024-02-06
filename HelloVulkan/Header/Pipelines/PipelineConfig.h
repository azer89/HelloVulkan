#ifndef PIPELINE_CONFIG
#define PIPELINE_CONFIG

#include "volk.h"

enum class PipelineType : uint8_t
{
	GraphicsOnScreen = 0u,
	GraphicsOffScreen = 1u,
	Compute = 2u,
};

struct PipelineConfig
{
	PipelineType type_ = PipelineType::GraphicsOnScreen;
	VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;
	VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	uint32_t PatchControlPointsCount_ = 0;
	bool vertexBufferBind_ = false;
	bool depthTest_ = true;
	bool depthWrite_ = true;
	bool useBlending_ = true;
};

#endif
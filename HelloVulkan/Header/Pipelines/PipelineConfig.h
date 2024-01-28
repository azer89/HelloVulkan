#ifndef PIPELINE_CONFIG
#define PIPELINE_CONFIG

#include "volk.h"

enum class PipelineFlags
{
	GraphicsOnScreen = 0,
	GraphicsOffScreen = 1,
	Compute = 2,
};

struct PipelineConfig
{
	PipelineFlags flags_ = PipelineFlags::GraphicsOnScreen;
	VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;
	VkPrimitiveTopology topology_ = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; // Defaults to triangles
	uint32_t PatchControlPointsCount_ = 0;
	bool vertexBufferBind_ = false;
	bool depthTest_ = true;
	bool depthWrite_ = true;
	bool useBlending_ = true;
};

#endif
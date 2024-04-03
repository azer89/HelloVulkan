#ifndef RAYTRACING_MODEL_DATA
#define RAYTRACING_MODEL_DATA

#include "VulkanBuffer.h"

struct RTModelData
{
	VulkanBuffer vertexBuffer_;
	VulkanBuffer indexBuffer_;
	VulkanBuffer transformBuffer_;
};

#endif
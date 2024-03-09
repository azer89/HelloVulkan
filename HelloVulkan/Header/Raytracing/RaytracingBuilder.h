#ifndef RAYTRACING_BUILDER
#define RAYTRACING_BUILDER

#include "VulkanContext.h"

class AccelStructure;
class VulkanBuffer;

class RaytracingBuilder
{
public:
	static void CreateBLAS(VulkanContext& ctx, 
		
		AccelStructure* blas);

	static void CreateTLAS(VulkanContext& ctx, AccelStructure* tlas);
};

#endif

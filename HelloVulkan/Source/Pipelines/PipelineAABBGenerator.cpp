#include "PipelineAABBGenerator.h"

PipelineAABBGenerator::PipelineAABBGenerator(
	VulkanDevice& vkDev, 
	ClusterForwardBuffers* cfBuffers,
	Camera* camera) :
	PipelineBase(vkDev,
	{
		.type_ = PipelineType::Compute
	}),
	cfBuffers_(cfBuffers),
	camera_(camera)
{
}


PipelineAABBGenerator::~PipelineAABBGenerator()
{

}
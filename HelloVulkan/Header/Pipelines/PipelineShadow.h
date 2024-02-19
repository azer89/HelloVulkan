#ifndef PIPELINE_SHADOW_MAPPING
#define PIPELINE_SHADOW_MAPPING

#include "PipelineBase.h"
#include "PushConstants.h"
#include "Model.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>

class PipelineShadow final : public PipelineBase
{
public:
	PipelineShadow(VulkanContext& ctx,
		std::vector<Model*> models,
		VulkanImage* shadowMap);
	~PipelineShadow();

	/*void SetShadowMapUBO(VulkanContext& ctx,
		glm::vec3 lightPosition, 
		glm::vec3 lightTarget,
		float nearPlane,
		float farPlane)*/
	void SetShadowMapUBO(VulkanContext& ctx, glm::mat4 lightSpaceMatrix)
	{
		uint32_t frameIndex = ctx.GetFrameIndex();
		
		//glm::mat4 lightProjection =
		//	glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, nearPlane, farPlane);
		//glm::mat4 lightView =
		//	glm::lookAt(lightPosition, lightTarget, glm::vec3(0.0, 1.0, 0.0));

		ShadowMapUBO ubo =
		{
			//.lightSpaceMatrix = lightProjection * lightView
			.lightSpaceMatrix = lightSpaceMatrix
		};

		shadowMapUBOBuffers_[frameIndex].UploadBufferData(ctx, &ubo, sizeof(ShadowMapUBO));
	}

	virtual void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

	void OnWindowResized(VulkanContext& ctx) override;
	
private:
	void CreateDescriptor(VulkanContext& ctx);

private:
	std::vector<Model*> models_;
	VulkanImage* shadowMap_;

	std::vector<VulkanBuffer> shadowMapUBOBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
};

#endif
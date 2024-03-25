#ifndef PIPELINE_SKYBOX
#define PIPELINE_SKYBOX

#include "PipelineBase.h"
#include "ResourcesShared.h"
#include "VulkanImage.h"
#include "Configs.h"

#include <array>

class PipelineSkybox final : public PipelineBase
{
public:
	PipelineSkybox(VulkanContext& ctx, 
		VulkanImage* envMap,
		ResourcesShared* resourcesShared,
		uint8_t renderBit = 0u
	);
	~PipelineSkybox();

	void SetCameraUBO(VulkanContext& ctx, CameraUBO& ubo) override
	{
		const uint32_t frameIndex = ctx.GetFrameIndex();
		
		// Remove translation
		CameraUBO skyboxUbo = ubo;
		skyboxUbo.view = glm::mat4(glm::mat3(skyboxUbo.view));

		cameraUBOBuffers_[frameIndex].UploadBufferData(ctx, &skyboxUbo, sizeof(CameraUBO));
	}

	void FillCommandBuffer(VulkanContext& ctx, VkCommandBuffer commandBuffer) override;

private:
	VulkanImage* envCubemap_;

	std::array<VkDescriptorSet, AppConfig::FrameCount> descriptorSets_;

	void CreateDescriptor(VulkanContext& ctx);
};

#endif

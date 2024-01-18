#ifndef RENDERER_AABB_DEBUG
#define RENDERER_AABB_DEBUG

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"
#include "ClusterForwardBuffers.h"
#include "UBO.h"
#include "Camera.h"

struct InverseViewUBO
{
	alignas(16)
	glm::mat4 cameraInverseView;
};

class RendererAABBDebug final : public RendererBase
{
public:
	RendererAABBDebug(
		VulkanDevice& vkDev,
		ClusterForwardBuffers* cfBuffers,
		VulkanImage* depthImage,
		VulkanImage* offscreenColorImage,
		uint8_t renderBit = 0u
	);
	~RendererAABBDebug();

	void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	void RenderEnable(bool enable) { shouldRender_ = enable; }

	void SetInverseCameraUBO(VulkanDevice& vkDev, Camera* camera)
	{
		InverseViewUBO invUBO
		{
			.cameraInverseView = glm::inverse(camera->GetViewMatrix())
		};
		UpdateUniformBuffer(vkDev.GetDevice(), invViewBuffer_, &invUBO, sizeof(InverseViewUBO));
		
	}

private:
	void CreateDescriptorLayoutAndSet(VulkanDevice& vkDev);

private:
	VulkanBuffer invViewBuffer_;
	ClusterForwardBuffers* cfBuffers_;
	std::vector<VkDescriptorSet> descriptorSets_;
	bool shouldRender_;
};

#endif
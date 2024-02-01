#ifndef RENDERER_AABB
#define RENDERER_AABB

#include "RendererBase.h"
#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "VulkanBuffer.h"
#include "VulkanUtility.h"
#include "ClusterForwardBuffers.h"
#include "Camera.h"
#include "UBO.h"

class RendererAABB final : public RendererBase
{
public:
	RendererAABB(VulkanDevice& vkDev, ClusterForwardBuffers* cfBuffers, Camera* camera);
	~RendererAABB();

	virtual void FillCommandBuffer(VulkanDevice& vkDev, VkCommandBuffer commandBuffer, size_t currentImage) override;

	//void UpdateCFUBO(const ClusterForwardUBO& ubo)
	//{
	//	cfUbo_ = ubo;
	//}

	void OnWindowResized(VulkanDevice& vkDev) override
	{
		//CreateClusters(vkDev);
		isDirty_ = true;
	}

private:
	bool isDirty_;
	ClusterForwardBuffers* cfBuffers_;
	Camera* camera_;

	VulkanBuffer cfUBOBuffer_;
	//ClusterForwardUBO cfUbo_;
	VkDescriptorSet computeDescriptorSet_;
	VkPipeline pipeline_;

private:
	void CreateClusters(VulkanDevice& vkDev, VkCommandBuffer commandBuffer);
	void Execute(VulkanDevice& vkDev, VkCommandBuffer commandBuffer);

	void CreateDescriptorPool(VkDevice device);
	void CreateDescriptorLayout(VkDevice device);

	void AllocateDescriptorSet(VkDevice device);
	void UpdateDescriptorSet(VkDevice device, VulkanBuffer* aabbBuffer);

	void CreateComputePipeline(
		VkDevice device,
		VkShaderModule computeShader);
};

#endif
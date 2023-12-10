#ifndef RENDERER_BASE
#define RENDERER_BASE

#define VK_NO_PROTOTYPES
#include "volk.h"

#include "VulkanDevice.h"
#include "VulkanImage.h"
#include "RenderPassCreateInfo.h"

class RendererBase
{
public:
	explicit RendererBase(const VulkanDevice& vkDev, VulkanImage depthTexture);
	virtual ~RendererBase();
	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) = 0;

	inline VulkanImage GetDepthTexture() const { return depthTexture_; }

protected:
	VkDevice device_ = nullptr;

	uint32_t framebufferWidth_ = 0;
	uint32_t framebufferHeight_ = 0;

	// Depth buffer
	VulkanImage depthTexture_;

	// Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
	VkDescriptorSetLayout descriptorSetLayout_ = nullptr;
	VkDescriptorPool descriptorPool_ = nullptr;
	std::vector<VkDescriptorSet> descriptorSets_;

	// Framebuffers (one for each command buffer)
	std::vector<VkFramebuffer> swapchainFramebuffers_;

	// 4. Pipeline & render pass (using DescriptorSets & pipeline state options)
	VkRenderPass renderPass_ = nullptr;
	VkPipelineLayout pipelineLayout_ = nullptr;
	VkPipeline graphicsPipeline_ = nullptr;

	// 5. Uniform buffer
	std::vector<VkBuffer> uniformBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;

protected:
	void BeginRenderPass(VkCommandBuffer commandBuffer, size_t currentImage);
	bool CreateUniformBuffers(VulkanDevice& vkDev, size_t uniformDataSize);

	bool CreateUniformBuffer(
		VulkanDevice& vkDev,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory,
		VkDeviceSize bufferSize);
	bool CreateBuffer(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkBuffer& buffer,
		VkDeviceMemory& bufferMemory);
	uint32_t FindMemoryType(VkPhysicalDevice device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

	bool CreateCubeTextureImage(
		VulkanDevice& vkDev, 
		const char* filename, 
		VkImage& textureImage, 
		VkDeviceMemory& textureImageMemory, 
		uint32_t* width = nullptr, 
		uint32_t* height = nullptr);

	bool CreateImageView(
		VkDevice device, 
		VkImage image, 
		VkFormat format, 
		VkImageAspectFlags aspectFlags, 
		VkImageView* imageView, 
		VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, 
		uint32_t layerCount = 1, 
		uint32_t mipLevels = 1);

	bool CreateTextureSampler(
		VkDevice device, 
		VkSampler* sampler, 
		VkFilter minFilter = VK_FILTER_LINEAR, 
		VkFilter maxFilter = VK_FILTER_LINEAR, 
		VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT);

	bool CreateColorAndDepthRenderPass(
		VulkanDevice& device, 
		bool useDepth, 
		VkRenderPass* renderPass, 
		const RenderPassCreateInfo& ci, 
		VkFormat colorFormat = VK_FORMAT_B8G8R8A8_UNORM);

	bool CreateColorAndDepthFramebuffers(
		VulkanDevice& vkDev, 
		VkRenderPass renderPass, 
		VkImageView depthImageView, 
		std::vector<VkFramebuffer>& swapchainFramebuffers);

	bool CreateDescriptorPool(
		VulkanDevice& vkDev, 
		uint32_t uniformBufferCount, 
		uint32_t storageBufferCount, 
		uint32_t samplerCount, 
		VkDescriptorPool* descriptorPool);

	bool CreatePipelineLayout(VkDevice device, 
		VkDescriptorSetLayout dsLayout, 
		VkPipelineLayout* pipelineLayout);

	bool CreateGraphicsPipeline(
		VulkanDevice& vkDev,
		VkRenderPass renderPass, 
		VkPipelineLayout pipelineLayout,
		const std::vector<const char*>& shaderFiles,
		VkPipeline* pipeline,
		VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST /* defaults to triangles*/,
		bool useDepth = true,
		bool useBlending = true,
		bool dynamicScissorState = false,
		int32_t customWidth = -1,
		int32_t customHeight = -1,
		uint32_t numPatchControlPoints = 0);
};

#endif
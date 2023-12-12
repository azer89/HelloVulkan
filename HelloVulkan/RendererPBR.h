#ifndef RENDERER_PBR
#define RENDERER_PBR

#include "RendererBase.h"
#include "VulkanTexture.h"
#include "Bitmap.h"

class RendererPBR : public RendererBase
{
public:
	RendererPBR(VulkanDevice& vkDev,
		uint32_t uniformBufferSize,
		const char* modelFile,
		const char* texAOFile,
		const char* texEmissiveFile,
		const char* texAlbedoFile,
		const char* texMeRFile,
		const char* texNormalFile,
		const char* texEnvMapFile,
		const char* texIrrMapFile,
		VulkanImage depthTexture);

	virtual ~RendererPBR();

	virtual void FillCommandBuffer(VkCommandBuffer commandBuffer, size_t currentImage) override;

	void UpdateUniformBuffer(VulkanDevice& vkDev, uint32_t currentImage, const void* data, const size_t dataSize);

private:
	bool CreateDescriptorSet(VulkanDevice& vkDev, uint32_t uniformDataSize);

	// ASSIMP
	bool CreatePBRVertexBuffer(
		VulkanDevice& vkDev, 
		const char* filename, 
		VkBuffer* storageBuffer, 
		VkDeviceMemory* storageBufferMemory, 
		size_t* vertexBufferSize, 
		size_t* indexBufferSize);

	// Textures
	void LoadTexture(VulkanDevice& vkDev, const char* fileName, VulkanTexture& texture);
	void LoadCubeMap(VulkanDevice& vkDev, const char* fileName, VulkanTexture& cubemap, uint32_t mipLevels = 1);

	bool CreateTextureImage(
		VulkanDevice& vkDev, 
		const char* filename, 
		VkImage& textureImage, 
		VkDeviceMemory& textureImageMemory, 
		uint32_t* outTexWidth = nullptr, 
		uint32_t* outTexHeight = nullptr);

	bool CreateMIPCubeTextureImage(
		VulkanDevice& vkDev, 
		const char* filename, 
		uint32_t mipLevels, 
		VkImage& textureImage, 
		VkDeviceMemory& textureImageMemory, 
		uint32_t* width = nullptr, 
		uint32_t* height = nullptr);

	bool CreateCubeTextureImage(
		VulkanDevice& vkDev, 
		const char* filename, 
		VkImage& textureImage, 
		VkDeviceMemory& textureImageMemory, 
		uint32_t* width = nullptr, 
		uint32_t* height = nullptr);

	bool CreateMIPTextureImageFromData(
		VulkanDevice& vkDev,
		VkImage& textureImage, 
		VkDeviceMemory& textureImageMemory,
		void* mipData, 
		uint32_t mipLevels, 
		uint32_t texWidth, 
		uint32_t texHeight,
		VkFormat texFormat,
		uint32_t layerCount = 1, 
		VkImageCreateFlags flags = 0);

	void CopyMIPBufferToImage(
		VulkanDevice& vkDev, 
		VkBuffer buffer, 
		VkImage image, 
		uint32_t mipLevels, 
		uint32_t width, 
		uint32_t height, 
		uint32_t bytesPP, 
		uint32_t layerCount = 1);

	Bitmap ConvertEquirectangularMapToVerticalCross(const Bitmap& b);

	Bitmap ConvertVerticalCrossToCubeMapFaces(const Bitmap& b);

private:
	size_t vertexBufferSize_;
	size_t indexBufferSize_;

	VkBuffer storageBuffer_;
	VkDeviceMemory storageBufferMemory_;

	VulkanTexture texAO_;
	VulkanTexture texEmissive_;
	VulkanTexture texAlbedo_;
	VulkanTexture texMeR_;
	VulkanTexture texNormal_;

	VulkanTexture envMapIrradiance_;
	VulkanTexture envMap_;

	VulkanTexture brdfLUT_;
};

#endif

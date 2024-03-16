#include "Mesh.h"

// Constructor
void Mesh::InitSlotBased(
	VulkanContext& ctx,
	uint32_t vertexOffset,
	uint32_t indexOffset,
	// Currently only support r-values
	std::vector<VertexData>&& _vertices,
	std::vector<uint32_t>&& _indices,
	std::unordered_map<TextureType, uint32_t>&& textureIndices)
{
	bindlessTexture_ = false;
	vertexOffset_ = vertexOffset;
	indexOffset_ = indexOffset; 
	vertices_ = std::move(_vertices);
	indices_ = std::move(_indices);
	textureIndices_ = std::move(textureIndices);
	vertexCount_ = vertices_.size();
	indexCount_ = indices_.size();

	SetupSlotBased(ctx);
}

void Mesh::InitBindless(
	VulkanContext& ctx,
	uint32_t vertexOffset,
	uint32_t indexOffset,
	uint32_t vertexCount,
	uint32_t indexCount,
	std::unordered_map<TextureType, uint32_t>&& textureIndices)
{
	bindlessTexture_ = true;
	vertexOffset_ = vertexOffset;
	indexOffset_ = indexOffset;
	vertexCount_ = vertexCount;
	indexCount_ = indexCount;
	textureIndices_ = std::move(textureIndices);
}

void Mesh::SetupSlotBased(VulkanContext& ctx)
{
	if (bindlessTexture_)
	{
		return;
	}

	VkDeviceSize vertexBufferSize = static_cast< VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		vertexBufferSize, 
		vertices_.data(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		indexBufferSize, 
		indices_.data(), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
	indexCount_ = static_cast<uint32_t>(indices_.size());
}

void Mesh::Destroy()
{	
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
}
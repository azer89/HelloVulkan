#include "Mesh.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

// Constructor
Mesh::Mesh(VulkanContext& ctx,
	uint32_t vertexOffset,
	uint32_t indexOffset,
	std::vector<VertexData>&& _vertices,
	std::vector<uint32_t>&& _indices,
	std::unordered_map<TextureType, uint32_t>&& textureIndices) :
	vertexOffset_(vertexOffset),
	indexOffset_(indexOffset),
	vertices_(std::move(_vertices)),
	indices_(std::move(_indices)),
	textureIndices_(std::move(textureIndices))
{
	Setup(ctx);
}

// Constructor
Mesh::Mesh(VulkanContext& ctx,
	uint32_t vertexOffset,
	uint32_t indexOffset,
	const std::vector<VertexData>& vertices,
	const std::vector<uint32_t>& indices,
	const std::unordered_map<TextureType, uint32_t>& textureIndices) :
	vertexOffset_(vertexOffset),
	indexOffset_(indexOffset),
	vertices_(vertices),
	indices_(indices),
	textureIndices_(textureIndices)
{
	Setup(ctx);
}

void Mesh::Setup(VulkanContext& ctx)
{
	vertexBufferSize_ = static_cast< VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		vertexBufferSize_, 
		vertices_.data(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	indexBufferSize_ = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		indexBufferSize_, 
		indices_.data(), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
}

void Mesh::Destroy()
{	
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
}
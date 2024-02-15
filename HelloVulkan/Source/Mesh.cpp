#include "Mesh.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

// Constructor
Mesh::Mesh(VulkanContext& vkDev,
	std::vector<VertexData>&& _vertices,
	std::vector<unsigned int>&& _indices,
	std::unordered_map<TextureType, VulkanImage*>&& _textures) :
	vertices_(std::move(_vertices)),
	indices_(std::move(_indices)),
	textures_(std::move(_textures))
{
	Setup(vkDev);
}

// Constructor
Mesh::Mesh(VulkanContext& vkDev,
	const std::vector<VertexData>& vertices,
	const std::vector<unsigned int>& indices,
	const std::unordered_map<TextureType, VulkanImage*>& textures) :
	vertices_(vertices),
	indices_(indices),
	textures_(textures)
{
	Setup(vkDev);
}

void Mesh::Setup(VulkanContext& vkDev)
{
	vertexBufferSize_ = sizeof(VertexData) * vertices_.size();
	vertexBuffer_.CreateGPUOnlyBuffer(
		vkDev, 
		vertexBufferSize_, 
		vertices_.data(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	indexBufferSize_ = sizeof(unsigned int) * indices_.size();
	indexBuffer_.CreateGPUOnlyBuffer(
		vkDev, 
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
#include "Mesh.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

#include <iostream>

// Constructor
Mesh::Mesh(VulkanDevice& vkDev,
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
Mesh::Mesh(VulkanDevice& vkDev,
	const std::vector<VertexData>& vertices,
	const std::vector<unsigned int>& indices,
	const std::unordered_map<TextureType, VulkanImage*>& textures) :
	vertices_(vertices),
	indices_(indices),
	textures_(textures)
{
	Setup(vkDev);
}

void Mesh::Setup(VulkanDevice& vkDev)
{
	vertexBufferSize_ = sizeof(VertexData) * vertices_.size();
	vertexBuffer_.CreateLocalMemoryBuffer(
		vkDev, 
		vertexBufferSize_, 
		vertices_.data(), 
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);

	indexBufferSize_ = sizeof(unsigned int) * indices_.size();
	indexBuffer_.CreateLocalMemoryBuffer(
		vkDev, 
		indexBufferSize_, 
		indices_.data(), 
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
	);
}

void Mesh::Destroy(VkDevice device)
{	
	vertexBuffer_.Destroy(device);
	indexBuffer_.Destroy(device);
}
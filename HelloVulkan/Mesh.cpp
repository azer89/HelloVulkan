#include "Mesh.h"

#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/cimport.h"

#include <iostream>

void Mesh::AddTexture(VulkanDevice& vkDev, const char* fileName, uint32_t bindIndex)
{
	VulkanTexture texture;
	texture.bindIndex = bindIndex;
	texture.CreateTextureImage(vkDev, fileName);
	texture.image.CreateImageView(
		vkDev.GetDevice(),
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT);
	texture.CreateTextureSampler(vkDev.GetDevice());
	textures_.push_back(texture);
}

void Mesh::Create(
	VulkanDevice& vkDev,
	const char* filename)
{
	const aiScene* scene = aiImportFile(filename, aiProcess_Triangulate);

	if (!scene || !scene->HasMeshes())
	{
		std::cerr << "Unable to load " << filename << '\n';
	}

	const aiMesh* mesh = scene->mMeshes[0];
	
	std::vector<VertexData> vertices;
	for (unsigned i = 0; i != mesh->mNumVertices; i++)
	{
		const aiVector3D v = mesh->mVertices[i];
		const aiVector3D t = mesh->mTextureCoords[0][i];
		const aiVector3D n = mesh->mNormals[i];
		vertices.push_back(
		{
			.pos = glm::vec4(v.x, v.y, v.z, 1.0f),
			.n = glm::vec4(n.x, n.y, n.z, 0.0f),
			.tc = glm::vec4(t.x, 1.0f - t.y, 0.0f, 0.0f) 
		});
	}

	std::vector<unsigned int> indices;
	for (unsigned i = 0; i != mesh->mNumFaces; i++)
	{
		for (unsigned j = 0; j != 3; j++)
			indices.push_back(mesh->mFaces[i].mIndices[j]);
	}
	aiReleaseImport(scene);

	vertexBufferSize_ = sizeof(VertexData) * vertices.size();
	indexBufferSize_ = sizeof(unsigned int) * indices.size();

	AllocateVertexBuffer(vkDev, vertices.data());
	AllocateIndexBuffer(vkDev, indices.data());

	AllocateSSBOBuffer(vkDev, vertices.data(), indices.data());
}

void Mesh::Destroy(VkDevice device)
{
	storageBuffer_.Destroy(device);
	vertexBuffer_.Destroy(device);
	indexBuffer_.Destroy(device);

	for (VulkanTexture& tex : textures_)
	{
		tex.DestroyVulkanTexture(device);
	}
}

void Mesh::AllocateVertexBuffer(
	VulkanDevice& vkDev,
	const void* vertexData)
{
	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		vertexBufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, vertexBufferSize_, 0, &data);
	memcpy(data, vertexData, vertexBufferSize_);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

	vertexBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		vertexBufferSize_,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vertexBuffer_.CopyFrom(vkDev, stagingBuffer.buffer_, vertexBufferSize_);

	stagingBuffer.Destroy(vkDev.GetDevice());
}

void Mesh::AllocateIndexBuffer(
	VulkanDevice& vkDev,
	const void* indexData)
{
	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		indexBufferSize_,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, indexBufferSize_, 0, &data);
	memcpy(data, indexData, indexBufferSize_);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

	indexBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		indexBufferSize_,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	indexBuffer_.CopyFrom(vkDev, stagingBuffer.buffer_, indexBufferSize_);

	stagingBuffer.Destroy(vkDev.GetDevice());
}

size_t Mesh::AllocateSSBOBuffer(
	VulkanDevice& vkDev,
	const void* vertexData,
	const void* indexData)
{
	VkDeviceSize bufferSize = vertexBufferSize_ + indexBufferSize_;

	VulkanBuffer stagingBuffer;
	stagingBuffer.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	);

	void* data;
	vkMapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_, 0, bufferSize, 0, &data);
	memcpy(data, vertexData, vertexBufferSize_);
	memcpy((unsigned char*)data + vertexBufferSize_, indexData, indexBufferSize_);
	vkUnmapMemory(vkDev.GetDevice(), stagingBuffer.bufferMemory_);

	storageBuffer_.CreateBuffer(
		vkDev.GetDevice(),
		vkDev.GetPhysicalDevice(),
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	storageBuffer_.CopyFrom(vkDev, stagingBuffer.buffer_, bufferSize);

	stagingBuffer.Destroy(vkDev.GetDevice());

	return bufferSize;
}
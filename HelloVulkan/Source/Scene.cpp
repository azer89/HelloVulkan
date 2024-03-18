#include "Scene.h"

#include "glm/glm.hpp"

#include <iostream>

Scene::Scene(VulkanContext& ctx, const std::vector<ModelData>& modelDataArray, bool supportDeviceAddress) :
	supportDeviceAddress_(supportDeviceAddress)
{
	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (const ModelData& mData : modelDataArray)
	{
		Model m;
		m.LoadBindless(
			ctx,
			mData,
			vertices_,
			indices_,
			vertexOffset,
			indexOffset);
		models_.push_back(m);
	}
	CreateBindlessResources(ctx);
}

Scene::~Scene()
{
	meshDataBuffer_.Destroy();
	vertexBuffer_.Destroy();
	indexBuffer_.Destroy();
	transformedBoundingBoxBuffer_.Destroy();
	for (auto& buffer : modelSSBOBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& buffer : indirectBuffers_)
	{
		buffer.Destroy();
	}
	for (auto& model : models_)
	{
		model.Destroy();
	}
}

VIM Scene::GetVIM() const
{
	return
	{
		.vertexBufferAddress = vertexBuffer_.deviceAddress_,
		.indexBufferAddress = indexBuffer_.deviceAddress_,
		.meshDataBufferAddress = meshDataBuffer_.deviceAddress_
	};
}

// TODO Create GPU only buffers
void Scene::CreateBindlessResources(VulkanContext& ctx)
{
	// Support for bindless rendering
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (supportDeviceAddress_) { bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; }

	// meshDataArray_ and modelInstanceToMeshData
	uint32_t matrixInstanceCount = 0u; // a counter and index to modelUBO_, it's also instance count for the scene
	uint32_t textureIndexOffset = 0u;
	modelInstanceToMeshData_.resize(models_.size()); // Mapping (modelIndex, instanceIndex) --> (meshDataArray_ index)
	for (size_t i = 0; i < models_.size(); ++i)
	{
		uint32_t instanceCount = models_[i].modelData_.instanceCount; // Duplicate count for this model
		modelInstanceToMeshData_[i].resize(instanceCount);
		for (int j = 0; j < instanceCount; ++j)
		{
			for (size_t k = 0; k < models_[i].meshes_.size(); ++k)
			{
				// matrixInstanceCount points to modelUBO_ array
				meshDataArray_.emplace_back(models_[i].meshes_[k].GetMeshData(textureIndexOffset, matrixInstanceCount));
			}
			modelInstanceToMeshData_[i][j] = matrixInstanceCount; // Set mapping
			matrixInstanceCount++;
		}
		textureIndexOffset += models_[i].GetTextureCount();
	}
	// Create a buffer for meshDataArray_
	const VkDeviceSize meshDataBufferSize = static_cast<VkDeviceSize>(sizeof(MeshData) * meshDataArray_.size());
	meshDataBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		meshDataBufferSize,
		meshDataArray_.data(), 
		bufferUsage);

	// Vertices
	const VkDeviceSize vertexBufferSize = static_cast<VkDeviceSize>(sizeof(VertexData) * vertices_.size());
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		vertexBufferSize,
		vertices_.data(),
		bufferUsage);

	// Indices
	const VkDeviceSize indexBufferSize = static_cast<VkDeviceSize>(sizeof(uint32_t) * indices_.size());
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		indexBufferSize,
		indices_.data(),
		bufferUsage);

	// ModelUBO which is actually an SSBO
	// Length of modelUBO_ is instance count
	modelUBOs_ = std::vector<ModelUBO>(matrixInstanceCount, { .model = glm::mat4(1.0f) });
	const VkDeviceSize modelSSBOBufferSize = sizeof(ModelUBO) * modelUBOs_.size();
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	modelSSBOBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelSSBOBuffers_[i].CreateBuffer(ctx, modelSSBOBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelSSBOBuffers_[i].UploadBufferData(ctx, modelUBOs_.data(), modelSSBOBufferSize);
	}

	BuildModelToMeshDataMapping();

	// Bounding boxes
	BuildBoundingBoxes(ctx);

	// Indirect buffers
	CreateIndirectBuffers(ctx, indirectBuffers_);
}

void Scene::BuildModelToMeshDataMapping()
{
	modelToMeshMap_.resize(models_.size());
	int iter = 0; // iter for all instances in the scene
	for (size_t i = 0; i < models_.size(); ++i)
	{
		size_t instanceCount = models_[i].modelData_.instanceCount;
		size_t instancedMeshCount = models_[i].GetMeshCount() * instanceCount;
		modelToMeshMap_[i].resize(instancedMeshCount);

		for (size_t j = 0; j < instancedMeshCount; ++j)
		{
			modelToMeshMap_[i][j] = iter++;
		}
	}
}

void Scene::BuildBoundingBoxes(VulkanContext& ctx)
{
	// Create bounding boxes
	originalBoundingBoxes_.resize(meshDataArray_.size());
	transformedBoundingBoxes_.resize(meshDataArray_.size());
	size_t boxIndex = 0; // bounding box index
	for (Model& model : models_)
	{
		glm::vec3 vmin(std::numeric_limits<float>::max());
		glm::vec3 vmax(std::numeric_limits<float>::lowest());

		size_t meshCount = model.GetMeshCount();
		size_t instanceCount = model.modelData_.instanceCount;

		// Create original bounding boxes with temporary array
		std::vector<BoundingBox> tempOriArray(meshCount);
		for (int i = 0; i < meshCount; ++i) // Per mesh
		{
			uint32_t vertexStart = model.meshes_[i].GetVertexOffset();
			uint32_t vertexEnd = vertexStart + model.meshes_[i].GetVertexCount();
			for (uint32_t j = vertexStart; j < vertexEnd; ++j)
			{
				const glm::vec3& v = vertices_[j].position;
				vmin = glm::min(vmin, v);
				vmax = glm::max(vmax, v);
			}
			tempOriArray[i].min_ = glm::vec4(vmin, 1.0);
			tempOriArray[i].max_ = glm::vec4(vmax, 1.0);
		}

		// Copy temporary array
		for (int i = 0; i < instanceCount; ++i)
		{
			for (int j = 0; j < meshCount; ++j)
			{
				originalBoundingBoxes_[boxIndex] = tempOriArray[j];
				const MeshData& mData = meshDataArray_[boxIndex];
				const glm::mat4& mat = modelUBOs_[mData.modelMatrixIndex].model;
				transformedBoundingBoxes_[boxIndex] = originalBoundingBoxes_[boxIndex].GetTransformed(mat);
				++boxIndex;
			}
		}
	}

	// Buffer
	VkDeviceSize bufferSize = static_cast<VkDeviceSize>(transformedBoundingBoxes_.size() * sizeof(BoundingBox));
	transformedBoundingBoxBuffer_.CreateBuffer(ctx, 
		bufferSize, 
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	transformedBoundingBoxBuffer_.UploadBufferData(ctx, transformedBoundingBoxes_.data(), bufferSize);
}

void Scene::CreateIndirectBuffers(
	VulkanContext& ctx,
	std::vector<VulkanBuffer>& indirectBuffers)
{
	const uint32_t instanceCount = meshDataArray_.size();
	const uint32_t indirectDataSize = instanceCount * sizeof(VkDrawIndirectCommand);
	constexpr size_t numFrames = AppConfig::FrameCount;
	const std::vector<uint32_t> meshVertexCountArray = GetInstanceVertexCountArray();

	indirectBuffers.resize(numFrames);
	for (size_t i = 0; i < numFrames; ++i)
	{
		indirectBuffers[i].CreateIndirectBuffer(ctx, indirectDataSize); // Create
		VkDrawIndirectCommand* data = indirectBuffers[i].MapIndirectBuffer(); // Map

		for (uint32_t j = 0; j < instanceCount; ++j)
		{
			data[j] =
			{
				.vertexCount = static_cast<uint32_t>(meshVertexCountArray[j]),
				.instanceCount = 1u,
				.firstVertex = 0,
				.firstInstance = j
			};
		}
		indirectBuffers[i].UnmapIndirectBuffer(); // Unmap
	}
}

void Scene::UpdateModelMatrix(VulkanContext& ctx,
	const ModelUBO& modelUBO,
	uint32_t modelIndex,
	uint32_t instanceIndex)
{
	if (modelIndex < 0 || modelIndex >= models_.size())
	{
		std::cerr << "Cannot update ModelUBO because of invalid modelIndex " << modelIndex << "\n";
		return;
	}

	uint32_t instanceCount = models_[modelIndex].modelData_.instanceCount;
	if (instanceIndex < 0 || instanceIndex >= instanceCount)
	{
		std::cerr << "Cannot update ModelUBO because of invalid instanceIndex " << instanceIndex << "\n";
		return;
	}

	int matrixIndex = modelInstanceToMeshData_[modelIndex][instanceIndex];

	// Update transformation matrix
	modelUBOs_[matrixIndex] = modelUBO;

	// Update SSBO
	for (uint32_t i = 0; i < AppConfig::FrameCount; ++i)
	{
		modelSSBOBuffers_[i].UploadOffsetBufferData(
			ctx,
			&modelUBO,
			sizeof(ModelUBO) * matrixIndex,
			sizeof(ModelUBO));
	}

	// Update bounding boxes
	std::vector<int>& meshIndices = modelToMeshMap_[modelIndex];
	if (meshIndices.size() > 0)
	{
		for (int i : meshIndices)
		{
			transformedBoundingBoxes_[i] = originalBoundingBoxes_[i].GetTransformed(modelUBO.model);
		}

		// Update bounding box buffers
		int firstIndex = meshIndices[0];
		transformedBoundingBoxBuffer_.UploadOffsetBufferData(
			ctx,
			transformedBoundingBoxes_.data() + firstIndex,
			sizeof(BoundingBox) * firstIndex,
			sizeof(BoundingBox) * meshIndices.size());
	}
}

// This is for descriptor indexing
std::vector<VkDescriptorImageInfo> Scene::GetImageInfos() const
{
	std::vector<VkDescriptorImageInfo> textureInfoArray;
	for (auto& model : models_)
	{
		for (auto& texture : model.textureList_)
		{
			textureInfoArray.emplace_back(texture.GetDescriptorImageInfo());
		}
	}
	return textureInfoArray;
}

// This is for indirect draw
std::vector<uint32_t> Scene::GetInstanceVertexCountArray() const
{
	std::vector<uint32_t> vCountArray(GetInstanceCount());
	size_t counter = 0;
	for (auto& model : models_)
	{
		for (int i = 0; i < model.modelData_.instanceCount; ++i)
		{
			for (auto& mesh : model.meshes_)
			{
				// Note that we use the index count here
				uint32_t indexCount = static_cast<uint32_t>(mesh.GetIndexCount());
				vCountArray[counter++] = indexCount;
			}
		}
	}
	return vCountArray;
}
#include "Scene.h"

#include "glm/glm.hpp"

#include <iostream>
#include <string>

Scene::Scene(VulkanContext& ctx, std::span<ModelCreateInfo> modelDataArray, bool supportDeviceAddress) :
	supportDeviceAddress_(supportDeviceAddress)
{
	uint32_t vertexOffset = 0u;
	uint32_t indexOffset = 0u;
	for (const ModelCreateInfo& mData : modelDataArray)
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
	indirectBuffer_.Destroy();
	for (auto& buffer : modelSSBOBuffers_)
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

void Scene::CreateBindlessResources(VulkanContext& ctx)
{
	CreateDataStructures();

	// Support for bindless rendering
	VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (supportDeviceAddress_) { bufferUsage |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT; }

	// Create a buffer for meshDataArray_
	const VkDeviceSize meshDataBufferSize = sizeof(MeshData) * meshDataArray_.size();
	meshDataBuffer_.CreateGPUOnlyBuffer(
		ctx, 
		meshDataBufferSize,
		meshDataArray_.data(), 
		bufferUsage);

	// Vertices
	const VkDeviceSize vertexBufferSize = sizeof(VertexData) * vertices_.size();
	vertexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		vertexBufferSize,
		vertices_.data(),
		bufferUsage);

	// Indices
	const VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices_.size();
	indexBuffer_.CreateGPUOnlyBuffer(
		ctx,
		indexBufferSize,
		indices_.data(),
		bufferUsage);

	// SSBO of ModelUBO
	const VkDeviceSize modelSSBOBufferSize = sizeof(ModelUBO) * modelSSBOs_.size();
	constexpr uint32_t frameCount = AppConfig::FrameCount;
	modelSSBOBuffers_.resize(frameCount);
	for (uint32_t i = 0; i < frameCount; ++i)
	{
		modelSSBOBuffers_[i].CreateBuffer(ctx, modelSSBOBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VMA_MEMORY_USAGE_CPU_TO_GPU);
		modelSSBOBuffers_[i].UploadBufferData(ctx, modelSSBOs_.data(), modelSSBOBufferSize);
	}

	// Bounding boxes
	const VkDeviceSize bbBufferSize = transformedBoundingBoxes_.size() * sizeof(BoundingBox);
	transformedBoundingBoxBuffer_.CreateBuffer(ctx,
		bbBufferSize,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU);
	transformedBoundingBoxBuffer_.UploadBufferData(ctx, transformedBoundingBoxes_.data(), bbBufferSize);

	// Indirect buffers
	CreateIndirectBuffer(ctx, indirectBuffer_);
}

void Scene::CreateDataStructures()
{
	uint32_t matrixCounter = 0u; // This will also be the length of modelSSBO_
	uint32_t textureCounter = 0u;
	uint32_t globalInstanceCounter = 0u;
	for (uint32_t m = 0; m < models_.size(); ++m)
	{
		const uint32_t meshCount = models_[m].GetMeshCount();
		const uint32_t instanceCount = models_[m].modelInfo_.instanceCount;

		// Create temporary bounding box array
		std::vector<BoundingBox> tempOriArray(meshCount);
		for (uint32_t i = 0; i < meshCount; ++i) // Per mesh
		{
			const uint32_t vertexStart = models_[m].meshes_[i].GetVertexOffset();
			const uint32_t vertexEnd = vertexStart + models_[m].meshes_[i].GetVertexCount();
			tempOriArray[i] = GetBoundingBox(vertexStart, vertexEnd);
		}

		// Create the actual InstanceData
		for (uint32_t i = 0; i < instanceCount; ++i)
		{
			for (uint32_t j = 0; j < meshCount; ++j)
			{
				instanceDataArray_.push_back(
				{
					.modelIndex = m,
					.meshIndex = j,
					.perModelInstanceIndex = i,
					.meshData = models_[m].meshes_[j].GetMeshData(textureCounter, matrixCounter),
					.originalBoundingBox = tempOriArray[j] // Copy bounding box from temporary
				}
				);
				++globalInstanceCounter;
			}
			++matrixCounter;
		}
		textureCounter += models_[m].GetTextureCount();
	}

	// For debugging purpose
	/*for (uint32_t i = 0; i < instanceDataArray_.size(); ++i)
	{
		if (i % 2 == 0)
		{
			instanceDataArray_[i].meshData.material_ = MaterialType::Transparent;
		}
	}*/
	// For debugging purpose
	/*for (uint32_t i = 0; i < instanceDataArray_.size(); ++i)
	{
		std::cout << std::to_string(static_cast<int>(instanceDataArray_[i].meshData.modelMatrixIndex_)) << " ";
	}
	std::cout << "\n\n";*/

	// Sort based on material
	std::sort(
		std::begin(instanceDataArray_),
		std::end(instanceDataArray_),
		[](InstanceData a, InstanceData b)
		{
			return a.meshData.material_ < b.meshData.material_;
		});
		
	// For debugging purpose
	/*for (uint32_t i = 0; i < instanceDataArray_.size(); ++i)
	{
		std::cout << std::to_string(static_cast<int>(instanceDataArray_[i].meshData.modelMatrixIndex_)) << " ";
	}*/

	// Matrices
	modelSSBOs_ = std::vector<ModelUBO>(matrixCounter, { .model = glm::mat4(1.0f) }); // Identity matrices
	
	// Flat array
	meshDataArray_.resize(globalInstanceCounter);
	for (uint32_t i = 0; i < instanceDataArray_.size(); ++i)
	{
		meshDataArray_[i] = instanceDataArray_[i].meshData;
	}

	// Initialize bounding boxes for frustum culling
	transformedBoundingBoxes_.resize(globalInstanceCounter);
	for (uint32_t i = 0; i < globalInstanceCounter; ++i)
	{
		// Equal the original bb because at this point modelSSBOs_ only has identity matrix
		transformedBoundingBoxes_[i] = instanceDataArray_[i].originalBoundingBox;
	}

	// Create a map
	matrixCounter = 0u;
	instanceMapArray_.resize(models_.size());
	for (size_t i = 0; i < models_.size(); ++i)
	{
		const uint32_t instanceCount = models_[i].modelInfo_.instanceCount;
		instanceMapArray_[i].resize(instanceCount);
		for (uint32_t j = 0; j < instanceCount; ++j)
		{
			instanceMapArray_[i][j].modelMatrixIndex = matrixCounter++;
		}
	}
	for (uint32_t i = 0; i < globalInstanceCounter; ++i)
	{
		const uint32_t modelIndex = instanceDataArray_[i].modelIndex;
		const uint32_t perModelInstanceIndex = instanceDataArray_[i].perModelInstanceIndex;
		instanceMapArray_[modelIndex][perModelInstanceIndex].boundingBoxIndices.push_back(i);
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

	const uint32_t instanceCount = models_[modelIndex].modelInfo_.instanceCount;
	if (instanceIndex < 0 || instanceIndex >= instanceCount)
	{
		std::cerr << "Cannot update ModelUBO because of invalid instanceIndex " << instanceIndex << "\n";
		return;
	}

	const int matrixIndex = instanceMapArray_[modelIndex][instanceIndex].modelMatrixIndex;

	// Update transformation matrix
	modelSSBOs_[matrixIndex] = modelUBO;

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
	std::vector<int>& boxIndices = instanceMapArray_[modelIndex][instanceIndex].boundingBoxIndices;
	if (!boxIndices.empty())
	{
		for (int i : boxIndices)
		{
			transformedBoundingBoxes_[i] = instanceDataArray_[i].originalBoundingBox.GetTransformed(modelUBO.model);
		}

		// Update bounding box buffers
		int firstIndex = boxIndices[0];
		transformedBoundingBoxBuffer_.UploadOffsetBufferData(
			ctx,
			transformedBoundingBoxes_.data() + firstIndex,
			sizeof(BoundingBox) * firstIndex,
			sizeof(BoundingBox) * boxIndices.size());
	}
}

BoundingBox Scene::GetBoundingBox(uint32_t vertexStart, uint32_t vertexEnd)
{
	glm::vec3 vMin(std::numeric_limits<float>::max());
	glm::vec3 vMax(std::numeric_limits<float>::lowest());
	for (uint32_t j = vertexStart; j < vertexEnd; ++j)
	{
		const glm::vec3& v = vertices_[j].position;
		vMin = glm::min(vMin, v);
		vMax = glm::max(vMax, v);
	}
	BoundingBox bb;
	bb.min_ = glm::vec4(vMin, 1.0);
	bb.max_ = glm::vec4(vMax, 1.0);
	return bb;
}

void Scene::CreateIndirectBuffer(
	VulkanContext& ctx,
	VulkanBuffer& indirectBuffer)
{
	const uint32_t instanceCount = static_cast<uint32_t>(meshDataArray_.size());
	const uint32_t indirectDataSize = instanceCount * sizeof(VkDrawIndirectCommand);
	//std::vector<uint32_t> meshVertexCountArray(instanceCount);// = GetInstanceVertexCountArray();

	/*for (uint32_t i = 0; i < instanceCount; ++i)
	{
		uint32_t modelIndex = instanceDataArray_[i].modelIndex;
		uint32_t meshIndex = instanceDataArray_[i].meshIndex;
		meshVertexCountArray[i] = models_[modelIndex].meshes_[meshIndex].GetIndexCount();
	}*/

	// Old code with map-able buffer, keeping it as reference
	/*indirectBuffer.CreateMappedIndirectBuffer(ctx, indirectDataSize); // Create
	VkDrawIndirectCommand* data = indirectBuffer.MapIndirectBuffer(); // Map

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
	indirectBuffer.UnmapIndirectBuffer(); // Unmap*/

	std::vector<VkDrawIndirectCommand> iCommands(instanceCount);
	for (uint32_t i = 0; i < instanceCount; ++i)
	{
		uint32_t modelIndex = instanceDataArray_[i].modelIndex;
		uint32_t meshIndex = instanceDataArray_[i].meshIndex;
		//meshVertexCountArray[i] = models_[modelIndex].meshes_[meshIndex].GetIndexCount();

		iCommands[i] =
		{
			.vertexCount = models_[modelIndex].meshes_[meshIndex].GetIndexCount(),
			.instanceCount = 1u,
			.firstVertex = 0,
			.firstInstance = i
		};
	}

	// This type of buffer is not accessible from CPU
	indirectBuffer.CreateGPUOnlyIndirectBuffer(ctx, iCommands.data(), indirectDataSize);
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
/*std::vector<uint32_t> Scene::GetInstanceVertexCountArray() const
{
	std::vector<uint32_t> vCountArray(GetInstanceCount());
	size_t counter = 0;
	for (auto& model : models_)
	{
		for (uint32_t i = 0; i < model.modelInfo_.instanceCount; ++i)
		{
			for (auto& mesh : model.meshes_)
			{
				// Note that we use the index count here
				vCountArray[counter++] = mesh.GetIndexCount();
			}
		}
	}
	return vCountArray;
}*/
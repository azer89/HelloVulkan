#version 460 core
#extension GL_EXT_buffer_reference : require

#include <ModelUBO.glsl>
#include <ShadowMapping//UBO.glsl>
#include <Bindless//VertexData.glsl>
#include <Bindless//MeshData.glsl>

// Bindless rendering
layout(std430, buffer_reference, buffer_reference_align = 4) 
readonly buffer VertexArray { VertexData vertices[]; };
layout(std430, buffer_reference, buffer_reference_align = 4) 
readonly buffer IndexArray { uint indices[]; };
layout(std430, buffer_reference, buffer_reference_align = 4) 
readonly buffer MeshDataArray { MeshData meshes[]; };

layout(push_constant) uniform Registers
{
	VertexArray vertexReference;
	IndexArray indexReference;
	MeshDataArray meshReference;
};

layout(set = 0, binding = 0) uniform UBOBlock { ShadowUBO shadowUBO; };
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };

void main()
{
	MeshData meshData = meshReference.meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = indexReference.indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = vertexReference.vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;

	// Output
	gl_Position = shadowUBO.lightSpaceMatrix * model * vertexData.position;
}
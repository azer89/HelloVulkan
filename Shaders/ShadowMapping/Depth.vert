#version 460 core
#extension GL_EXT_buffer_reference : require

// ShadowMapping/Depth.vert 

#include <ModelUBO.glsl>
#include <ShadowMapping//UBO.glsl>
#include <Bindless//VertexData.glsl>
#include <Bindless//MeshData.glsl>

//layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer VertexDataBlock
//{
//	VertexData vertex;
//};

layout(std430, buffer_reference, buffer_reference_align = 4) readonly buffer VertexArray
{
	VertexData vertexArray[];
};

layout(std430,buffer_reference,buffer_reference_align = 4) readonly buffer IndexArray
{
	uint indexArray[];
};

layout(push_constant) uniform Registers
{
	VertexArray vertexReference;
	IndexArray indexReference;
};
//registers;

layout(set = 0, binding = 0) uniform UBOBlock { ShadowUBO shadowUBO; };
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs []; }; // SSBO
//layout(set = 0, binding = 2) readonly buffer Vertices { VertexData vertices []; }; // SSBO
layout(set = 0, binding = 2) readonly buffer Indices { uint indices []; }; // SSBO
layout(set = 0, binding = 3) readonly buffer Meshes { MeshData meshes []; }; // SSBO

void main()
{
	MeshData meshData = meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;

	//uint vIndex = indices[iOffset + gl_VertexIndex] + vOffset;
	restrict uint vIndex = indexReference.indexArray[iOffset + gl_VertexIndex] + vOffset;

	restrict VertexData vertexData = vertexReference.vertexArray[vIndex];

	//VertexData vertexData = vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;

	// Output
	gl_Position = shadowUBO.lightSpaceMatrix * model * vertexData.position;
}
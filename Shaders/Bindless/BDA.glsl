
// Bindless rendering
layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer VertexArray { VertexData vertices []; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer IndexArray { uint indices []; };

layout(std430, buffer_reference, buffer_reference_align = 4)
readonly buffer MeshDataArray { MeshData meshes []; };

struct BDA
{
	// These hold the addresses of the buffers
	VertexArray vertexReference;
	IndexArray indexReference;
	MeshDataArray meshReference;
	uint _pad1;
	uint _pad2;
};
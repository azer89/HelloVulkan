#version 460 core

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out flat uint meshIndex;

// UBO
layout(set = 0, binding = 0)
#include <CameraUBO.frag>

// SSBO
struct ModelUBO
{
	mat4 model;
};
layout(set = 0, binding = 1) readonly buffer ModelUBOs { ModelUBO modelUBOs[]; };

// SSBO
#include <Bindless//VertexData.frag>
layout(set = 0, binding = 2) readonly buffer Vertices { VertexData vertices[]; };

// SSBO
layout(set = 0, binding = 3) readonly buffer Indices { uint indices[]; };

// SSBO
#include <Bindless//MeshData.frag>
layout(set = 0, binding = 4) readonly buffer Meshes { MeshData meshes[]; };

void main()
{
	MeshData meshData = meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = vertices[vIndex];

	mat4 model = modelUBOs[gl_BaseInstance].model;

	mat3 normalMatrix = transpose(inverse(mat3(model)));

	worldPos = (model * vertexData.position).xyz;
	texCoord = vertexData.uv.xy;
	normal = normalMatrix * vertexData.normal.xyz;
	meshIndex = gl_BaseInstance;

	gl_Position =  camUBO.projection * camUBO.view * vec4(worldPos, 1.0);
}

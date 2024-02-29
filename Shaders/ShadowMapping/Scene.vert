#version 460 core

/*
Vertex shader for 
	* PBR+IBL
	* Naive forward shading (non clustered)
	* Shadow mapping
	* Bindless
*/

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec4 shadowPos;
layout(location = 4) out flat uint meshIndex;

// UBO
layout(set = 0, binding = 0)
#include <CameraUBO.glsl>

// UBO
layout(set = 0, binding = 1)
#include <ShadowMapping//UBO.glsl>

// SSBO
struct ModelUBO
{
	mat4 model;
};
layout(set = 0, binding = 2) readonly buffer ModelUBOs { ModelUBO modelUBOs []; };

// SSBO
#include <Bindless//VertexData.glsl>
layout(set = 0, binding = 3) readonly buffer Vertices { VertexData vertices []; };

// SSBO
layout(set = 0, binding = 4) readonly buffer Indices { uint indices []; };

// SSBO
#include <Bindless//MeshData.glsl>
layout(set = 0, binding = 5) readonly buffer Meshes { MeshData meshes []; };

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

void main()
{
	MeshData meshData = meshes[gl_BaseInstance];
	uint vOffset = meshData.vertexOffset;
	uint iOffset = meshData.indexOffset;
	uint vIndex = indices[iOffset + gl_VertexIndex] + vOffset;
	VertexData vertexData = vertices[vIndex];
	mat4 model = modelUBOs[meshData.modelMatrixIndex].model;
	mat3 normalMatrix = transpose(inverse(mat3(model)));

	// Output
	worldPos = (model * vertexData.position).xyz;
	texCoord = vertexData.uv.xy;
	normal = normalMatrix * vertexData.normal.xyz;
	meshIndex = gl_BaseInstance;
	shadowPos = biasMat * shadowUBO.lightSpaceMatrix * model * vertexData.position;
	gl_Position =  camUBO.projection * camUBO.view * vec4(worldPos, 1.0);
}
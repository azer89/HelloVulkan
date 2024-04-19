#version 460 core

#include <CameraUBO.glsl>
#include <InfiniteGrid/Params.glsl>

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec2 outCameraPos;

layout(push_constant) uniform PC
{
	// This sets the height of the grid
	float yPosition;
};

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };

void main()
{
	mat4 mvp = camUBO.projection * camUBO.view;
	int idx = VERTEX_INDICES[gl_VertexIndex];
	
	vec3 position = VERTEX_POS[idx] * GRID_EXTENTS;
	position.y = yPosition;
	
	outCameraPos = camUBO.position.xy;

	position.x += camUBO.position.x;
	position.z += camUBO.position.y;

	outUV = position.xz;
	gl_Position = mvp * vec4(position, 1.0);
}
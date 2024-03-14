#version 460 core

#include <CameraUBO.glsl>
#include <InfiniteGrid//Params.glsl>

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec2 outCameraPos;

layout(push_constant) uniform PC
{
	// Need to adjust because Blender grid is off
	float yPosition;
};

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };

void main()
{
	mat4 mvp = camUBO.projection * camUBO.view;
	int idx = VERTEX_INDICES[gl_VertexIndex];
	
	vec3 position = VERTEX_POS[idx] * GRID_SIZE;
	position.y = yPosition;
	
	outCameraPos = camUBO.position.xy;
	
	outUV = position.xz;
	gl_Position = mvp * vec4(position, 1.0);
	

}
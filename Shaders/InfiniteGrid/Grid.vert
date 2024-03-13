#version 460 core

#include <CameraUBO.glsl>
#include <InfiniteGrid//Params.glsl>

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec2 outCameraPos;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };

void main()
{
	mat4 mvp = camUBO.projection * camUBO.view;
	int idx = indices[gl_VertexIndex];
	vec3 position = VERTEX_POS[idx] * gridSize;
	mat4 invView = inverse(camUBO.view);
	//outCameraPos = vec2(invView[3][0], invView[3][2]);
	outCameraPos = camUBO.position.xy;
	//position.x += outCameraPos.x;
	//position.z += outCameraPos.y;
	outUV = position.xz;
	position.y = -1;
	gl_Position = mvp * vec4(position, 1.0);
	

}
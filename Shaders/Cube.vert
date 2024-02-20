#version 460 core

/*
Vertex shader to generate a cube
*/

layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0)
#include <CameraUBO.frag>

const vec3 pos[8] = vec3[8]
(
	vec3(-1.0, -1.0, 1.0),
	vec3( 1.0, -1.0, 1.0),
	vec3( 1.0,  1.0, 1.0),
	vec3(-1.0,  1.0, 1.0),

	vec3(-1.0, -1.0, -1.0),
	vec3( 1.0, -1.0, -1.0),
	vec3( 1.0,  1.0, -1.0),
	vec3(-1.0,  1.0, -1.0)
);

const int indices[36] = int[36]
(
	// Front
	0, 1, 2, 2, 3, 0,
	// Right
	1, 5, 6, 6, 2, 1,
	// Back
	7, 6, 5, 5, 4, 7,
	// Left
	4, 0, 3, 3, 7, 4,
	// Bottom
	4, 5, 1, 1, 0, 4,
	// Top
	3, 2, 6, 6, 7, 3
);

void main()
{
	int idx = indices[gl_VertexIndex];
	vec4 pos4 = vec4(pos[idx], 1.0);

	// depthCompareOp should be VK_COMPARE_OP_LESS_OR_EQUAL
	gl_Position = (camUBO.projection * camUBO.view * pos4).xyww;
	
	direction = pos[idx].xyz;
}

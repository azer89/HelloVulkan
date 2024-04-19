#version 460 core

#include <CameraUBO.glsl>
#include <Cube.glsl>

/*
Vertex shader to generate a skybox
*/

layout(location = 0) out vec3 direction;

layout(set = 0, binding = 0)  uniform CameraBlock { CameraUBO camUBO; };

void main()
{
	int idx = CUBE_INDICES[gl_VertexIndex];
	vec4 pos4 = vec4(CUBE_POS[idx], 1.0);

	// depthCompareOp should be VK_COMPARE_OP_LESS_OR_EQUAL
	gl_Position = (camUBO.projection * camUBO.view * pos4).xyww;
	
	direction = CUBE_POS[idx].xyz;
}

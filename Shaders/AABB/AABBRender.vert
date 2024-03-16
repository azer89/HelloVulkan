#version 460 core

#include <Cube.glsl>
#include <CameraUBO.glsl>
#include <AABB/AABB.glsl>

layout(set = 0, binding = 0)  uniform CameraBlock { CameraUBO camUBO; };
layout(set = 0, binding = 1) buffer Boxes { AABB boxes[]; };

void main()
{
	AABB box = boxes[gl_InstanceIndex];
	float extents = length(box.maxPoint - box.minPoint) * 0.5;
	int idx = CUBE_INDICES[gl_VertexIndex];
	vec4 pos4 = vec4(CUBE_POS[idx] * extents, 1.0);
	gl_Position = camUBO.projection * camUBO.view * pos4;

}
#version 460 core

#include <Cube.glsl>
#include <CameraUBO.glsl>
#include <AABB/AABB.glsl>

layout(set = 0, binding = 0)  uniform CameraBlock { CameraUBO camUBO; };
layout(set = 0, binding = 1) buffer Boxes { AABB boxes[]; };

// Render an AABB (not rotated)
void main()
{
	AABB box = boxes[gl_InstanceIndex];
	vec3 extents = (box.maxPoint - box.minPoint).xyz * 0.5;
	vec3 boxPos = (box.maxPoint + box.minPoint).xyz * 0.5;
	int idx = CUBE_INDICES[gl_VertexIndex];
	vec3 cubePos = CUBE_POS[idx];
	cubePos = vec3(cubePos.x * extents.x, cubePos.y * extents.y, cubePos.z * extents.z);
	cubePos += boxPos;
	gl_Position = camUBO.projection * camUBO.view * vec4(cubePos, 1.0);

}
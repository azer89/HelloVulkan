#version 460 core

#include <CameraUBO.glsl>

struct PointColor
{
	vec4 position;
	vec4 color;
};

layout(location = 0) out vec4 lineColor;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };
layout(set = 0, binding = 1) readonly buffer PointBlock { PointColor points[]; };

void main()
{
	PointColor pc = points[gl_VertexIndex];
	gl_Position = camUBO.projection * camUBO.view * pc.position;
	lineColor = pc.color;
}
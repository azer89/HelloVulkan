#version 460 core

#include <CameraUBO.glsl>
#include <InfiniteGrid//Params.glsl>

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };

void main()
{
}
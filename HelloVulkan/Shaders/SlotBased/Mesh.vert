#version 460 core

// SlotBased/Mesh.vert

#include <CameraUBO.glsl>
#include <ModelUBO.glsl>

layout(location = 0) in vec3 inPosition;
layout(location = 1) in float inUVX;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in float inUVY;
layout(location = 4) in vec4 inColor;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;

layout(set = 0, binding = 0) uniform CameraBlock { CameraUBO camUBO; };
layout(set = 0, binding = 1) uniform ModelUBOBlock { ModelUBO modelUBO; };

void main()
{
	mat3 normalMatrix = transpose(inverse(mat3(modelUBO.model)));

	texCoord = vec2(inUVX, inUVY);
	normal = normalMatrix * inNormal;
	worldPos = (modelUBO.model * vec4(inPosition, 1.0)).xyz;
	gl_Position =  camUBO.projection * camUBO.view * vec4(worldPos, 1.0);
}

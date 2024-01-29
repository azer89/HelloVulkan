#version 460 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;

layout(location = 0) out vec3 viewPos;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec2 texCoord;
layout(location = 3) out vec3 normal;

layout(set = 0, binding = 0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
	vec4 position;
	float near;
	float far;
}
camUBO;

layout(set = 0, binding = 1) uniform ModelUBO
{
	mat4 model;
}
modelUBO;

void main()
{
	mat3 normalMatrix = transpose(inverse(mat3(modelUBO.model)));

	texCoord = inUV.xy;
	normal = normalMatrix * inNormal.xyz;
	worldPos = (modelUBO.model * inPosition).xyz;
	viewPos = (camUBO.view * modelUBO.model * inPosition).xyz;

	gl_Position =  camUBO.projection * vec4(viewPos, 1.0);
}

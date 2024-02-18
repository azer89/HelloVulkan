#version 460 core

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inNormal;
layout(location = 2) in vec4 inUV;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;
layout(location = 3) out vec4 shadowPos;

layout(set = 0, binding = 0) uniform CameraUBO
{
	mat4 projection;
	mat4 view;
	vec4 position;
}
camUBO;

layout(set = 0, binding = 1) uniform ModelUBO
{
	mat4 model;
}
modelUBO;

layout(set = 0, binding = 2) uniform ShadowMapConfigUBO
{
	mat4 lightSpaceMatrix;
	vec4 lightPosition;
	float shadowMapSize;
	float shadowMinBias;
	float shadowMaxBias;
	float shadowNearPlane;
	float shadowFarPlane;
}
shadowUBO;

const mat4 biasMat = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0);

void main()
{
	mat3 normalMatrix = transpose(inverse(mat3(modelUBO.model)));

	texCoord = inUV.xy;
	normal = normalMatrix * inNormal.xyz;
	worldPos = (modelUBO.model * inPosition).xyz;
	shadowPos = biasMat * shadowUBO.lightSpaceMatrix * modelUBO.model * inPosition;
	gl_Position =  camUBO.projection * camUBO.view * vec4(worldPos, 1.0);
}

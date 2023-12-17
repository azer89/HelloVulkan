# version 460 core

layout(location = 0) in vec4 positionIn;
layout(location = 1) in vec4 normalIn;
layout(location = 2) in vec4 uvIn;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec2 texCoord;
layout(location = 2) out vec3 normal;

layout(binding = 0) uniform UniformBuffer
{
	mat4 mvp;
	mat4 mv;
	mat4 m;
	vec4 cameraPos;
}
ubo;

void main()
{
	worldPos = (ubo.m * positionIn).xyz;

	texCoord = uvIn.xy;

	mat3 normalMatrix = transpose(inverse(mat3(ubo.m)));
	normal = normalMatrix * normalIn.xyz;

	gl_Position = ubo.mvp * positionIn;
}

#version 460 core

layout(location = 0) in vec3 dir;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform samplerCube texture1;

void main()
{
    fragColor = texture(texture1, dir);
};

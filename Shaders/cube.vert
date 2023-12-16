#version 460 core

layout(location = 0) out vec3 dir;

layout(binding = 0) uniform CameraUBO
{
    mat4 mvp;
}
camUBO;

const vec3 pos[8] = vec3[8](
    vec3(-1.0, -1.0, 1.0),
    vec3(1.0, -1.0, 1.0),
    vec3(1.0, 1.0, 1.0),
    vec3(-1.0, 1.0, 1.0),

    vec3(-1.0, -1.0, -1.0),
    vec3(1.0, -1.0, -1.0),
    vec3(1.0, 1.0, -1.0),
    vec3(-1.0, 1.0, -1.0)
);

const int indices[36] = int[36]
(
    // Front
    0, 1, 2, 2, 3, 0,
    // Right
    1, 5, 6, 6, 2, 1,
    // Back
    7, 6, 5, 5, 4, 7,
    // Left
    4, 0, 3, 3, 7, 4,
    // Bottom
    4, 5, 1, 1, 0, 4,
    // Top
    3, 2, 6, 6, 7, 3
);

void main()
{
    float cubeSize = 20.0;
    int idx = indices[gl_VertexIndex];
    gl_Position = camUBO.mvp * vec4(cubeSize * pos[idx], 1.0);
    dir = pos[idx].xyz;
}

# version 460 core

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D colorImage;

void main()
{
	vec3 color = texture(colorImage, texCoord).rgb;

	fragColor = vec4(color, 1.0);
};

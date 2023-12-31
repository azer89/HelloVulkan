# version 460 core

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(binding = 0) uniform sampler2D colorImage;

void main()
{
	vec3 color = texture(colorImage, texCoord).rgb;

	// HDR tonemapping and gamma correction
	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0 / 2.2));

	fragColor = vec4(color, 1.0);
};

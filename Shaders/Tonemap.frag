# version 460 core

/*
[1] https://github.com/Nadrin/PBR/
[2] "Photographic Tone Reproduction for Digital Images", Equation 4
*/

layout(location = 0) in vec2 texCoord;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform sampler2D colorImage;

// TODO Push constants
const float GAMMA = 2.2;
const float PURE_WHITE = 1.0;

vec3 ReinhardTonemap(vec3 color)
{
	float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
	float mappedLuminance =
		(luminance * (1.0 + luminance / (PURE_WHITE * PURE_WHITE))) / (1.0 + luminance);

	// Scale color by ratio of average luminances
	vec3 mappedColor = (mappedLuminance / luminance) * color;

	// Gamma correction
	return pow(mappedColor, vec3(1.0 / GAMMA));
}

void main()
{
	vec3 color = texture(colorImage, texCoord).rgb;

	vec3 mappedColor = ReinhardTonemap(color);

	// Gamma correction
	fragColor = vec4(mappedColor, 1.0);
};
struct CameraProperties
{
	mat4 projInverse;
	mat4 viewInverse;
	vec4 position;
	uint frame; // Used for blending/Anti aliasing
};
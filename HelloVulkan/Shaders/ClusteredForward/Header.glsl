struct LightCell
{
	uint offset;
	uint count;
};

struct ClusterForwardUBO
{
	mat4 cameraInverseProjection;
	mat4 cameraView;
	vec2 screenSize;
	float sliceScaling;
	float sliceBias;
	float cameraNear;
	float cameraFar;
	uint sliceCountX;
	uint sliceCountY;
	uint sliceCountZ;
};
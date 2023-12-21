# version 460 core

layout(location = 0) out vec2 texCoord;

void main()
{
	float x = float((gl_VertexIndex & 1) << 2);
	float y = float((gl_VertexIndex & 2) << 1);
	texCoord.x = x * 0.5;
	texCoord.y = y * 0.5;
	gl_Position = vec4(x - 1.0, y - 1.0, 0, 1);
}
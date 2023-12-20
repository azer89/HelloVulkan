# version 460 core

layout(location = 0) out vec2 v_UV;

void main()
{
	/*float x = -1.0 + float((gl_VertexIndex & 1) << 2);
	float y = -1.0 + float((gl_VertexIndex & 2) << 1);
	v_UV.x = (x+1.0)*0.5;
	v_UV.y = (y+1.0)*0.5;
	gl_Position = vec4(x, y, 0.0, 1.0);*/

	v_UV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	gl_Position = vec4(v_UV * 2.0f + -1.0f, 0.0f, 1.0f);
}
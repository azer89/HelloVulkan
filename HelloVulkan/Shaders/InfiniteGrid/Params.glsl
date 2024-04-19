/*
Adapted from
	3D Graphics Rendering Cookbook
	by Sergey Kosarevsky & Viktor Latypov
	github.com/PacktPublishing/3D-Graphics-Rendering-Cookbook
*/

// Extents of grid in world coordinates
const float GRID_EXTENTS = 50.0;

// Size of one cell
const float GRID_CELL_SIZE = 0.2;

// Color of thin lines
const vec4 GRID_COLOR_THIN = vec4(0.2, 0.2, 0.2, 1.0);

// Color of thick lines (every tenth line)
const vec4 GRID_COLOR_THICK = vec4(0.0, 0.0, 0.0, 1.0);

const vec3 VERTEX_POS[4] = vec3[4](
	vec3(-1.0, 0.0, -1.0),
	vec3( 1.0, 0.0, -1.0),
	vec3( 1.0, 0.0,  1.0),
	vec3(-1.0, 0.0,  1.0)
);

const int VERTEX_INDICES[6] = int[6](
	0, 1, 2, 2, 3, 0
);

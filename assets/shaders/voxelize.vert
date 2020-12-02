
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_normal;
layout (location = 3) in vec3 i_xtan;
layout (location = 4) in vec3 i_ytan;

uniform mat4 u_transform;

out vec3 a_pos;
out vec3 a_normal;
out vec3 a_xtan;
out vec3 a_ytan;
out vec2 a_uv;

void main() {
	/* calculates the position in world coordinates by applying the 
	transform matrix */
	vec4 pos = u_transform * vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = pos;

	/* extracts the rotation matrix from the transform matrix, we do this 
	because the normal needs to just be rotated, not translated. (this also 
	applies scaling to the normal, but we can normalize the vector later) */
	mat3 rotate = mat3(transpose(inverse(u_transform)));

	a_normal = rotate * i_normal;
	a_xtan = rotate * i_xtan;
	a_ytan = rotate * i_ytan;

	a_uv = i_uv;
}
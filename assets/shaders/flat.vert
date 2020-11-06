
layout (location = 0) in vec3 i_position;
layout (location = 1) in vec2 i_uv;
layout (location = 2) in vec3 i_normal;

out vec3 a_pos;
out vec3 a_normal;
out vec2 a_uv;

uniform mat4 u_project;
uniform mat4 u_transform;
uniform mat4 u_rotate;

void main() {
	vec4 pos = u_transform * vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = u_project * pos;

	mat3 rotate = mat3(transpose(inverse(u_transform)));

	a_normal = rotate * i_normal;
}
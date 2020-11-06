
layout (location = 0) in vec3 i_position;

out vec3 a_pos;

uniform mat4 u_project;

void main() {
	vec4 pos = vec4(i_position, 1.0f);
	a_pos = pos.xyz;
	gl_Position = u_project * pos;
}
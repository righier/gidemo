#version 460 core

layout (location = 0) in vec3 i_position;

uniform mat4 u_project;
uniform mat4 u_transform;

void main() {
	gl_Position = u_project * u_transform * vec4(i_position, 1.0f);
}
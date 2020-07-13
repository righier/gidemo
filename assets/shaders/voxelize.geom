#version 460 core

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 a_pos[];
in vec3 a_posv[];
in vec4 a_posls[];
in vec3 a_normal[];
in vec2 a_uv[];

out vec3 b_pos;
out vec3 b_posv;
out vec4 b_posls;
out vec3 b_normal;
out vec2 b_uv;

void main(){
	const vec3 a = a_pos[1] - a_pos[0];
	const vec3 b = a_pos[2] - a_pos[0];
	const vec3 p = abs(cross(a, b)); 
	for(uint i = 0; i < 3; ++i){
		b_pos = a_pos[i];
		b_posv = a_posv[i];
		b_posls = a_posls[i];
		b_normal = a_normal[i];
		b_uv = a_uv[i];
		if(p.z > p.x && p.z > p.y){
			gl_Position = vec4(gl_Position.x, gl_Position.y, 0, 1);
		} else if (p.x > p.y && p.x > p.z){
			gl_Position = vec4(gl_Position.y, gl_Position.z, 0, 1);
		} else {
			gl_Position = vec4(gl_Position.x, gl_Position.z, 0, 1);
		}
		EmitVertex();
	}
    EndPrimitive();
}
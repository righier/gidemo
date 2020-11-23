
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 a_pos[];
in vec3 a_normal[];
in vec3 a_xtan[];
in vec3 a_ytan[];
in vec2 a_uv[];

out vec3 b_pos;
out vec3 b_normal;
out vec3 b_xtan;
out vec3 b_ytan;
out vec2 b_uv;

void main(){
	const vec3 a = a_pos[1] - a_pos[0];
	const vec3 b = a_pos[2] - a_pos[0];
	const vec3 p = abs(cross(a, b)); 
	for(uint i = 0; i < 3; ++i){
		vec4 pos = gl_in[i].gl_Position;
		if(p.z > p.x && p.z > p.y){
			gl_Position = vec4(pos.x, pos.y, 0, 1);
		} else if (p.x > p.y){
			gl_Position = vec4(pos.y, pos.z, 0, 1);
		} else {
			gl_Position = vec4(pos.x, pos.z, 0, 1);
		}
		b_pos = a_pos[i];
		b_normal = a_normal[i];
		b_xtan = a_xtan[i];
		b_ytan = a_ytan[i];
		b_uv = a_uv[i];

		EmitVertex();
	}
    EndPrimitive();
}
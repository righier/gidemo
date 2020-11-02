
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
		} else if (p.x > p.y && p.x > p.z){
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


/*
// hPixel = dimensions of half a pixel cell    
// Compute equations of the planes through the two edges  
vec3 plane[2];  
plane[0] = cross(currentPos.xyw - prevPos.xyw, prevPos.xyw);  
plane[1] = cross(nextPos.xyw - currentPos.xyw, currentPos.xyw);    
// Move the planes by the appropriate semidiagonal  
plane[0].z -= dot(hPixel.xy, abs(plane[0].xy));  
plane[1].z -= dot(hPixel.xy, abs(plane[1].xy));      
// Compute the intersection point of the planes.  
float4 finalPos;  
finalPos.xyw = cross(plane[0], plane[1]); 
*/

/*
//semiDiagonal[0,1] = Normalized semidiagonals in the same  
// quadrants as the edge's normals.  
// vtxPos = The position of the current vertex.  
// hPixel = dimensions of half a pixel cell    
float dp = dot(semiDiagonal[0], semiDiagonal[1]);  
float2 diag;      
if (dp > 0) {    // The normals are in the same quadrant -> One vertex    
	diag = semiDiagonal[0];  
}  else if(dp < 0) {    // The normals are in neighboring quadrants -> Two vertices      
	diag = (In.index == 0 ? semiDiagonal[0] : semiDiagonal[1]);  
} else {    // The normals are in opposite quadrants -> Three vertices    
	if (In.index == 1) {      // Special "quadrant between two quadrants case"      
		diag = float2(
			semiDiagonal[0].x * semiDiagonal[0].y * semiDiagonal[1].x,
			semiDiagonal[0].y * semiDiagonal[1].x * semiDiagonal[1].y);    
	} else {
		diag = (In.index == 0 ? semiDiagonal[0] : semiDiagonal[1]);  
	}
}
vtxPos.xy += hPixel.xy * diag * vtxPos.w;
*/
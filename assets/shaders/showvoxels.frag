#version 460 core

in vec3 a_pos;
// in vec2 a_uv;
// in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform sampler3D u_voxelTexture;

uniform vec3 u_cameraPos;
uniform float u_voxelScale;
uniform int u_voxelCount;
uniform float u_voxelLod;

layout(location = 0) out vec3 o_albedo;

bool inside(vec3 v) {
  v = abs(v);
  return max(max(v.x, v.y), v.z) <= 1.0f;
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5/u_voxelScale)+0.5;  
}

float getMaxComponent(vec3 p) {
  vec3 q = abs(p);
  return max(max(q.x, q.y), q.z);
}

void main() {
  // if (gl_FragCoord.z <= 0) return;

  vec3 camv = toVoxel(u_cameraPos);
  vec3 posv = toVoxel(a_pos);
  vec3 dir = normalize(posv - camv);

  float hi = length(posv - camv);
  float lo = 0.0f;
  for (int i = 0; i < 10; i++) {
    float mid = (hi + lo) * 0.5f;
    if (insideVoxel(camv + dir*mid)) hi = mid;
    else lo = mid;
  }

  vec3 start = camv + dir * hi;

  vec4 color = vec4(0);

  float stepSize = pow(2, u_voxelLod) / (u_voxelCount*5.0f);

  vec3 pos = start;

  int i = 0;
  int maxi = 1000;
  while (insideVoxel(pos) && color.a < 0.99f) {
    vec4 cc = textureLod(u_voxelTexture, pos, u_voxelLod);
    color += (1.0f - color.a) * cc;
    pos += dir*stepSize;
  }

  o_albedo = pow(color.rgb, vec3(1.0/2.2));
}
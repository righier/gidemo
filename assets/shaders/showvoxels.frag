#version 460 core

in vec3 a_pos;
// in vec2 a_uv;
// in vec3 a_normal;
// in vec3 a_xtan;
// in vec3 a_ytan;


uniform vec3 u_cameraPos;
uniform int u_voxelCount;
uniform float u_voxelLod;
uniform int u_voxelIndex;

float voxelSize = 1.0f / u_voxelCount;

uniform sampler3D u_voxelTexture[6];
const ivec3 DIR = ivec3(0, 2, 4);

layout(location = 0) out vec3 o_albedo;

bool inside(vec3 v) {
  v = abs(v);
  return max(max(v.x, v.y), v.z) <= 1.0f;
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

float getMaxComponent(vec3 p) {
  vec3 q = abs(p);
  return max(max(q.x, q.y), q.z);
}

vec4 fetch(vec3 dir, vec3 pos, float lod) {
  vec3 d = -dir;
  bvec3 sig = lessThanEqual(d, vec3(0.0f));
  ivec3 bd = DIR + ivec3(lessThan(d, vec3(0.0f)));
  vec3 sd = d * d; 

  vec4 cx = sig.x ? textureLod(u_voxelTexture[1], pos, lod) : textureLod(u_voxelTexture[0], pos, lod);
  vec4 cy = sig.y ? textureLod(u_voxelTexture[3], pos, lod) : textureLod(u_voxelTexture[2], pos, lod);
  vec4 cz = sig.z ? textureLod(u_voxelTexture[5], pos, lod) : textureLod(u_voxelTexture[4], pos, lod);

  return sd.x * cx + sd.y * cy + sd.z * cz;

  // return sd.x * textureLod(u_voxelTexture[bd.x], pos, lod) + sd.y * textureLod(u_voxelTexture[bd.y], pos, lod) + sd.z * textureLod(u_voxelTexture[bd.z], pos, lod);
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

  float stepSize = pow(2, u_voxelLod) / (u_voxelCount*10.0f);
  stepSize = 0.001;

  vec3 pos = start;

  int lod = int(u_voxelLod);
  vec3 dim = vec3(float(u_voxelCount)/pow(2, float(lod)));
  int i = 0;
  int maxi = 1000;
  while (insideVoxel(pos) && color.a < 0.99f) {

    // vec3 pp = vec3(ivec3(pos*dim)) / dim + voxelSize*(1 << lod) / 2;
    // vec4 cc = textureLod(u_voxelTexture[u_voxelIndex], pp, u_voxelLod);
    vec4 cc = texelFetch(u_voxelTexture[u_voxelIndex], ivec3(pos*dim), lod);
    // vec4 cc = fetch(dir, pos, u_voxelLod);
    color += (1.0f - color.a) * vec4(cc.xyz, 1.0f) * cc.a;
    pos += dir*stepSize;
  }

  // if (color.a <= 0.01f) discard;

  o_albedo = pow(color.rgb, vec3(1.0/2.2));
}
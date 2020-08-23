#version 460 core

in vec3 a_pos;
in vec2 a_uv;
in vec3 a_normal;
in vec4 a_posls;
// in vec3 a_xtan;
// in vec3 a_ytan;

uniform vec3 u_diffuse;
uniform float u_metal;
uniform float u_rough;
uniform float u_opacity;
uniform vec3 u_emission;

// uniform sampler2DArray u_tex;

uniform int u_voxelCount;

uniform vec3 u_cameraPos;

uniform sampler3D u_voxelTexture[6];
const ivec3 DIR = ivec3(0, 2, 4);

uniform float u_offsetDist;
uniform float u_offsetPos;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform float u_shadowBias;

uniform sampler2D u_shadowMap;

layout(location = 0) out vec3 o_albedo;

float voxelSize = 1.0f / float(u_voxelCount);

vec3 ortho(vec3 v) {
  vec3 w = normalize(v + vec3(0,0,1));
  return cross(v, w);
}

bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) && all(greaterThanEqual(pos, vec3(0.0f)));
}

vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

vec4 fetch(vec3 dir, vec3 pos, float lod) {
  vec3 d = dir;
  bvec3 sig = lessThan(d, vec3(0.0f));
  ivec3 bd = DIR + ivec3(lessThan(d, vec3(0.0f)));
  vec3 sd = d * d; 

  vec4 cx = sig.x ? textureLod(u_voxelTexture[1], pos, lod) : textureLod(u_voxelTexture[0], pos, lod);
  vec4 cy = sig.y ? textureLod(u_voxelTexture[3], pos, lod) : textureLod(u_voxelTexture[2], pos, lod);
  vec4 cz = sig.z ? textureLod(u_voxelTexture[5], pos, lod) : textureLod(u_voxelTexture[4], pos, lod);

  return sd.x * cx + sd.y * cy + sd.z * cz;

  // return sd.x * textureLod(u_voxelTexture[bd.x], pos, lod) + sd.y * textureLod(u_voxelTexture[bd.y], pos, lod) + sd.z * textureLod(u_voxelTexture[bd.z], pos, lod);
}

/*
  float dist = 0.1953125;
  dist *= u_offsetDist;

    // Trace.
  while(dist < SQRT2 && acc.a < 1){
    vec3 c = (from + dist * direction)*0.5 + 0.5;
    float l = (1 + CONE_SPREAD * dist / voxelSize);
    float level = log2(l);
    float ll = (level + 1) * (level + 1);
    vec4 voxel = textureLod(u_voxelTexture, c, min(MIPMAP_HARDCAP, level));
    acc += 0.075 * ll * voxel * pow(1 - voxel.a, 2);
    dist += ll * voxelSize * 2;
  }
  return pow(acc.rgb * 2.0, vec3(1.5));
*/

float traceShadow(vec3 start, vec3 lightPos) {
  // start = toVoxel(start);
  // lightPos = toVoxel(lightPos);
  vec3 dir = lightPos - start;
  float lightDist = length(dir);
  dir /= lightDist;
  
  float acc = 0;

  float dist = 3*voxelSize;

  while (dist < lightDist && acc < 1.0) {
    vec3 pos = start + dir * dist;
    float l = pow(dist, 2);
    float s1 = 0.062 * textureLod(u_voxelTexture[0], toVoxel(pos), 1+0.75 * l).a;
    float s2 = 0.135 * textureLod(u_voxelTexture[0], toVoxel(pos), 4.5 * l).a;
    // float s = s1 + s2;
    float s = textureLod(u_voxelTexture[0], toVoxel(pos), 0.0).a;

    acc += (1-acc) * s;
    dist += 0.9 * voxelSize * (1 + 0.05 * l);
    // dist += voxelSize;

  }

  return pow(smoothstep(0, 1, acc * 1.4), 1.0f / 1.4);
  // return acc;
}

vec3 traceDiffuse(const vec3 start, const vec3 dir, float aperture, float maxdist) {
  vec4 acc = vec4(0.f);
  vec3 pos;
  float dist = voxelSize * u_offsetDist;
  float lod = -1.f;
  float diam = 0.5f * voxelSize;

  for (int i = 0; i < 7; i++) {
    pos = start + dir * dist;
    diam *= 2.f;
    lod += 1.0f;
    vec4 cc = fetch(dir, pos, max(0.002f, lod));
    acc += (1.f - acc.w) * cc;
    dist += diam;
  }

  return acc.xyz;
}

/*
vec3 traceDiffuse(const vec3 start, vec3 dir, float aperture, float maxdist) {
  vec4 acc = vec4(0);
  float invsize = 1.0f / float(u_voxelCount);

  float offset = u_offsetPos;
  vec3 from = start + dir * (offset * invsize);
  // float dist = 4*(u_offsetDist/float(u_voxelCount));

  float dist = 0.1953125;
  dist *= u_offsetDist;
  vec3 pos = from + dist * dir;
  float aperture2 = aperture * 2.0f;

  while (dist < maxdist && acc.a < 1.f) {

    float diam = max(voxelSize*0.5, aperture2 * dist);
    float mipmap = max(0.002, log2(diam * float(u_voxelCount)));
    // float mipmap = log2(diam * float(u_voxelCount));
    // float mipmap = max(0.002, log2(diam * float(u_voxelCount)) );
    // vec4 cc = textureLod(u_voxelTexture[0], pos, mipmap);
    vec4 cc = fetch(dir, pos, mipmap-1.f);
    cc = fetch(dir, pos, 7);
    // cc = fetch(dir, vec3(0), 7);
    // acc += (1.0f - acc.a) * cc;
    acc += cc * pow(1 - cc.a, 2);
    dist += max(diam, voxelSize) * 0.5;

    // dist += voxelSize * 0.5;
    pos = from + dist * dir;

    // pos += dir * (1.0f/u_voxelCount)*0.2;
  }

  return acc.xyz;
}
*/


vec3 indirectDiffuse2() {
  const float coneAngle = radians(45.f);
  const float cs = sin(coneAngle);
  const float cc = cos(coneAngle);
  const int NCONES = 7;
  const vec3 cones[NCONES] = {
    vec3(0, 0, 1),
    vec3(cs, 0, cc),
    vec3(-cs, 0, cc),
    vec3(cs*cc, cs*cs, cc),
    vec3(cs*cc, -cs*cs, cc),
    vec3(-cs*cc, cs*cs, cc),
    vec3(-cs*cc, -cs*cs, cc)
  };

  vec3 normal = a_normal;
  vec3 xtan = ortho(normal);
  vec3 ytan = cross(normal, xtan);
  mat3 tanSpace = mat3(xtan, ytan, normal); 

  vec3 acc = vec3(0);

  float offset = u_offsetPos;
  vec3 pos = toVoxel(a_pos) + normal*(offset/u_voxelCount);

  float maxdist = sqrt(3.f);

  // acc = traceDiffuse(pos, normal, tan(coneAngle/2.0f), maxdist);

  for (int i = 0; i < NCONES; i++) {
    vec3 dir = tanSpace * cones[i];
    acc += traceDiffuse(pos, dir, tan(coneAngle/2.0f), maxdist);
  }

  return acc;
}

vec3 traceDiffuseVoxelCone(const vec3 from, vec3 direction){
  direction = normalize(direction);
  
  const float CONE_SPREAD = 0.325;
  const float MIPMAP_HARDCAP = 7.f;
  const float SQRT2 = 1.414213;

  vec4 acc = vec4(0.0f);

  // Controls bleeding from close surfaces.
  // Low values look rather bad if using shadow cone tracing.
  // Might be a better choice to use shadow maps and lower this value.
  float dist = 0.1953125;
  dist *= u_offsetDist;

    // Trace.
  while(dist < SQRT2 && acc.a < 1){
    vec3 c = (from + dist * direction)*0.5 + 0.5;
    float l = (1 + CONE_SPREAD * dist / voxelSize);
    float level = log2(l);
    float ll = (level + 1) * (level + 1);
    vec4 voxel = fetch(direction, c, min(MIPMAP_HARDCAP, level));
    acc += 0.075 * ll * voxel * pow(1 - voxel.a, 2);
    dist += ll * voxelSize * 2;
    // dist += CONE_SPREAD * dist * sqrt(2) + voxelSize;
  }
  return pow(acc.rgb * 2.0, vec3(1.5));
}

vec3 indirectDiffuse(){
  const float ANGLE_MIX = 0.5f; // Angle mix (1.0f => orthogonal direction, 0.0f => direction of normal).

  const float w[3] = {1.0, 0.5, 0.5}; // Cone weights.

  vec3 normal = a_normal;
  vec3 worldPositionFrag = a_pos;
  float voxelSize = 1.0f / float(u_voxelCount);

  // Find a base for the side cones with the normal as one of its base vectors.
  const vec3 ortho = normalize(ortho(normal));
  const vec3 ortho2 = normalize(cross(ortho, normal));

  // Find base vectors for the corner cones too.
  const vec3 corner = 0.5f * (ortho + ortho2);
  const vec3 corner2 = 0.5f * (ortho - ortho2);

  // Find start position of trace (start with a bit of offset).
  const vec3 N_OFFSET = normal * (1 + 4 * 0.707106) * voxelSize;
  const vec3 C_ORIGIN = worldPositionFrag + N_OFFSET;

  // Accumulate indirect diffuse light.
  vec3 acc = vec3(0);

  // We offset forward in normal direction, and backward in cone direction.
  // Backward in cone direction improves GI, and forward direction removes
  // artifacts.
  const float CONE_OFFSET = u_offsetPos;

  // Trace front cone
  acc += w[0] * traceDiffuseVoxelCone(C_ORIGIN + CONE_OFFSET * normal, normal);

  // return acc

  #if 1
  // Trace 4 side cones.
  const vec3 s1 = mix(normal, ortho, ANGLE_MIX);
  const vec3 s2 = mix(normal, -ortho, ANGLE_MIX);
  const vec3 s3 = mix(normal, ortho2, ANGLE_MIX);
  const vec3 s4 = mix(normal, -ortho2, ANGLE_MIX);

  acc += w[1] * traceDiffuseVoxelCone(C_ORIGIN + CONE_OFFSET * ortho, s1);
  acc += w[1] * traceDiffuseVoxelCone(C_ORIGIN - CONE_OFFSET * ortho, s2);
  acc += w[1] * traceDiffuseVoxelCone(C_ORIGIN + CONE_OFFSET * ortho2, s3);
  acc += w[1] * traceDiffuseVoxelCone(C_ORIGIN - CONE_OFFSET * ortho2, s4);

  // Trace 4 corner cones.
  const vec3 c1 = mix(normal, corner, ANGLE_MIX);
  const vec3 c2 = mix(normal, -corner, ANGLE_MIX);
  const vec3 c3 = mix(normal, corner2, ANGLE_MIX);
  const vec3 c4 = mix(normal, -corner2, ANGLE_MIX);

  acc += w[2] * traceDiffuseVoxelCone(C_ORIGIN + CONE_OFFSET * corner, c1);
  acc += w[2] * traceDiffuseVoxelCone(C_ORIGIN - CONE_OFFSET * corner, c2);
  acc += w[2] * traceDiffuseVoxelCone(C_ORIGIN + CONE_OFFSET * corner2, c3);
  acc += w[2] * traceDiffuseVoxelCone(C_ORIGIN - CONE_OFFSET * corner2, c4);
  #endif

  // Return result.
  return acc;
}




float shadow() {
  float bias = u_shadowBias;
  vec3 coords = a_posls.xyz / a_posls.w;
  coords = coords*0.5 + 0.5;
  float shadowDepth = texture(u_shadowMap, coords.xy).r;
  float pointDepth = coords.z;
  float shadowVal = pointDepth >= shadowDepth + bias ? 1.0 : 0.0;
  return shadowVal;
}

/*
// Returns a soft shadow blend by using shadow cone tracing.
// Uses 2 samples per step, so it's pretty expensive.
float traceShadowCone(vec3 from, vec3 direction, float targetDistance){
  float voxelSize = 1.0f / u_voxelCount;
  from += normal * 0.05f; // Removes artifacts but makes self shadowing for dense meshes meh.

  float acc = 0;

  float dist = 3 * voxelSize;
  // I'm using a pretty big margin here since I use an emissive light ball with a pretty big radius in my demo scenes.
  const float STOP = targetDistance - 16 * voxelSize;

  while(dist < STOP && acc < 1){  
    vec3 c = from + dist * direction;
    if(!insideVoxel(c)) break;
    c = scaleAndBias(c);
    float l = pow(dist, 2); // Experimenting with inverse square falloff for shadows.
    float s1 = 0.062 * textureLod(u_voxelTexture, c, 1 + 0.75 * l).a;
    float s2 = 0.135 * textureLod(u_voxelTexture, c, 4.5 * l).a;
    float s = s1 + s2;
    acc += (1 - acc) * s;
    dist += 0.9 * voxelSize * (1 + 0.05 * l);
  }
  return 1 - pow(smoothstep(0, 1, acc * 1.4), 1.0 / 1.4);
} 
*/

vec3 getDirectionWeights(vec3 direction){
#if 0
  vec3 d = abs(normalize(direction));
  return d / dot(d, vec3(1.0));
#elif 0
  return abs(direction);
#else
  return direction * direction;
#endif
}

/*
vec4 fetch(vec3 dir, vec3 pos, float lod) {
  vec3 d = -dir;
  bvec3 sig = lessThan(d, vec3(0.0f));
  ivec3 bd = DIR + ivec3(lessThan(d, vec3(0.0f)));
  vec3 sd = d * d; 

  vec4 cx = sig.x ? textureLod(u_voxelTexture[1], pos, lod) : textureLod(u_voxelTexture[0], pos, lod);
  vec4 cy = sig.y ? textureLod(u_voxelTexture[3], pos, lod) : textureLod(u_voxelTexture[2], pos, lod);
  vec4 cz = sig.z ? textureLod(u_voxelTexture[5], pos, lod) : textureLod(u_voxelTexture[4], pos, lod);

  return sd.x * cx + sd.y * cy + sd.z * cz;

  // return sd.x * textureLod(u_voxelTexture[bd.x], pos, lod) + sd.y * textureLod(u_voxelTexture[bd.y], pos, lod) + sd.z * textureLod(u_voxelTexture[bd.z], pos, lod);
}
*/
vec4 traceVoxelCone( vec3 from, vec3 direction, float aperture, float offset, float maxDistance){
  direction = normalize(direction);
  bvec3 negativeDirection = lessThan(direction, vec3(0.0));
  float doubledAperture = max(voxelSize, 2.0 * aperture),
  dist = offset;

  vec3 directionWeights = getDirectionWeights(direction),

  position = from + (dist * direction);
  vec4 accumulator = vec4(0.0);
  maxDistance = min(maxDistance, 1.41421356237);
  while((dist < maxDistance) && (accumulator.a < 1.0)){
    float diameter = max(voxelSize * 0.5, doubledAperture * dist),
    mipMapLevel = max(0.0, log2((diameter * float(u_voxelCount)) + 0.0));
    vec4 voxel = fetch(direction, position, mipMapLevel);
    accumulator += (1.0 - accumulator.w) * voxel;
    dist += max(diameter, voxelSize) * 0.5;
    position = from + (dist * direction);
  }
  return max(accumulator, vec4(0.0));
}

float traceShadowCone(vec3 normal, vec3 from, vec3 to){
  const float aperture = tan(radians(5.0)),
  doubledAperture = max(voxelSize, 2.0 * aperture),

  s = 0.5;
  vec3 direction = normalize(to - from);
  from += direction * voxelSize * 4.0;
  float maxDistance = length(to - from);
  float dist = 0 * voxelSize;
  float accumulator = 0.0;
  // direction /= maxDistance;
  maxDistance = min(maxDistance, 1.41421356237);
  // dist += voxelJitterNoise(vec4(from.xyz + to.xyz + normal.xyz, tc.x)).x * s * voxelSize;
  vec3 position = from + (direction * dist);
  while((accumulator < 1.0) && (dist < maxDistance)){
    float diameter = max(voxelSize * 0.5, doubledAperture * dist);
    if (dist+diameter >= maxDistance) break;
    float mipMapLevel = max(0.0, log2((diameter * float(u_voxelCount)) + 1.0));
    float cc = fetch(direction, position, mipMapLevel).w;
    accumulator += (1.0 - accumulator) * cc;
    dist += max(diameter, voxelSize) * s;
    position = from + (direction * dist);
  }
  return 1.0 - accumulator;
  // return clamp(1.0 - accumulator, 0.0, 1.0);
}

float traceShadow() {
  vec3 normal = a_normal;
  vec3 pos = toVoxel(a_pos);
  vec3 lightPos = toVoxel(u_lightPos);
  return traceShadowCone(normal, pos, lightPos);
}

vec3 traceSpecular() {
  vec4 acc = vec4(0);

  vec3 pos = toVoxel(a_pos);
  vec3 camPos = toVoxel(u_cameraPos);
  vec3 normal = a_normal;

  vec3 camDir = normalize(pos - camPos);
  vec3 dir = reflect(camDir, normal);


  const float coneAngle = radians(5.f);

  pos += normal * voxelSize * u_offsetPos;

  float maxdist = sqrt(3.f);
  float distOffset = voxelSize * 3.0; //u_offsetDist;

  float aperture = tan(coneAngle/2.0f);

  return traceVoxelCone(pos, dir, aperture, distOffset, maxdist).xyz;

}


vec3 spotlight() {
  vec3 spotDir = u_lightDir;
  vec3 lightDir = normalize(u_lightPos - a_pos);
  float dist = abs(length(a_pos - u_lightPos));
  float angle = dot(spotDir, -lightDir);
  float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
  float attenuation = 1.0 / (1.0 + dist); 
  intensity *= attenuation;
  intensity *= max(0, dot(a_normal, lightDir));
  // intensity *= 1.0 - min(1.0f, traceShadow(a_pos, u_lightPos));
  // intensity *= 1.0 - min(1.0f, shadow());
  if (intensity > 0.01) {
    intensity *= traceShadow();
  }
  return u_lightColor * intensity;
}


void main() {
  // o_albedo = texture(u_tex, vec3(a_uv, 0)).rgb;
  // o_metal_rough_ao.r = texture(u_tex, vec3(a_uv, 2)).r;
  // o_metal_rough_ao.g = texture(u_tex, vec3(a_uv, 3)).r;
  // o_metal_rough_ao.b = 1.0f;

  // mat3 tangent_matrix = mat3(a_xtan, a_ytan, a_normal);
  // vec3 t_normal = texture(u_tex, vec3(a_uv, 1)).xyz * 2 - 1;
  // o_normal = tangent_matrix * t_normal;



  // vec3 directLight = spotlight();
  vec3 indirectLight = indirectDiffuse();
  vec3 specular = traceSpecular();

  // float shadow2 = 1.0 - shadow();
  vec3 spot = spotlight();

  vec3 color = u_diffuse;

  // color = directLight + indirectLight;
  // color *= u_diffuse;
  // color = specular * shadow;
  // color = spot * u_diffuse + specular;

  // color = specular;
  vec3 diffuseLight = indirectLight + spot;
  color = u_diffuse * (diffuseLight * 0.7) + specular * 0.3;

  color = indirectLight;

  // vec4 voxelDiffuse = textureLod(u_voxelTexture, toVoxel(a_pos), 0.1);
  // color = vec3(voxelDiffuse);

  o_albedo = pow(color, vec3(1.0/2.2));
  // o_albedo = color;
}
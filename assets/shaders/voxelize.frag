
in vec3 b_pos;
in vec3 b_normal;
in vec3 b_xtan;
in vec3 b_ytan;
in vec4 b_posls;
in vec2 b_uv;

uniform vec3 u_diffuse;
uniform vec3 u_emission;
uniform float u_emissionScale;
uniform float u_metal;
uniform float u_rough;

uniform bool u_particle;
uniform float u_opacity;

uniform bool u_useMaps;
uniform sampler2D u_diffuseMap;
uniform sampler2D u_bumpMap;
uniform sampler2D u_emissionMap;

uniform mat4 u_lightProj;
uniform float u_shadowBias;

uniform vec3 u_lightPos;
uniform vec3 u_lightDir;
uniform vec3 u_lightColor;
uniform float u_lightInnerCos;
uniform float u_lightOuterCos;

uniform bool u_averageValues;
uniform float u_restitution;

uniform int u_voxelCount;
float voxelSize = 1.0f / float(u_voxelCount);

uniform bool u_temporalMultibounce;

/* the time uniform is used to generate random values */
uniform float u_time;
float time = mod(u_time, 1.0f);

uniform sampler2D u_shadowMap;

layout(RGBA8) uniform image3D u_voxelTexture;

#ifdef ANISO
uniform sampler3D u_voxelTexturePrev[6];
#else
uniform sampler3D u_voxelTexturePrev;
#endif

const float PI = 3.14159265359;

/* wrapper to fetch both isotropic and anisotropic voxels */
vec4 fetch(vec3 dir, vec3 pos, float lod) {
#ifdef ANISO
  /* we need to know for every axis the direction to sample from */
  bvec3 sig = greaterThan(dir, vec3(0.0f));
  float l2 = lod; 

  vec4 cx = sig.x ? textureLod(u_voxelTexturePrev[1], pos, l2) 
                  : textureLod(u_voxelTexturePrev[0], pos, l2);
  vec4 cy = sig.y ? textureLod(u_voxelTexturePrev[3], pos, l2) 
                  : textureLod(u_voxelTexturePrev[2], pos, l2);
  vec4 cz = sig.z ? textureLod(u_voxelTexturePrev[5], pos, l2) 
                  : textureLod(u_voxelTexturePrev[4], pos, l2);
  vec3 sd = dir * dir; 
  vec4 c1 = sd.x * cx + sd.y * cy + sd.z * cz;
  return c1;
#else
  return textureLod(u_voxelTexturePrev, pos, max(1, lod));
#endif
}

/* shadows with PCF */
float shadow() {
  vec4 coords = u_lightProj * vec4(b_pos, 1.0);
  float bias = u_shadowBias;
  coords = coords*0.5;
  coords += coords.w;
  float total = 0;
  const int nsamples = 3; /* the real sample number is nsample*nsample */
  float scale = voxelSize/2; /* how far is the farthest sample from the origin */
  for (int i = 0; i < nsamples; i++) {
    for (int j = 0; j < nsamples; j++) {
      vec2 offset = vec2(i, j) / nsamples * 2 - 1;
      offset *= scale;
      vec3 pos = coords.xyw + vec3(offset, 0);

      float shadowDepth = textureProj(u_shadowMap, pos).r;
      float pointDepth = coords.z / coords.w; /* normalize depth */
      float val = float(pointDepth - bias >= shadowDepth);
      total += val;
    }
  }

  /* average of samples */
  total /= nsamples*nsamples;

  /* to prevent floating point errors */
  return clamp(total, 0, 1);
}

/* calculates the light contribution coming from the spotlight 
 * taking into account the light color, the light attenuation, 
 * shadows and NdotL*/
vec3 spotlight(vec3 normal) {
  vec3 spotDir = u_lightDir;
  vec3 lightDir = u_lightPos - b_pos;
  float dist = abs(length(lightDir));
  lightDir = lightDir / dist;
  float angle = dot(spotDir, -lightDir);
  float intensity = smoothstep(u_lightOuterCos, u_lightInnerCos, angle);
  float attenuation = 1.0 / (1.0 + dist);	
  intensity *= attenuation;
  intensity *= max(0, dot(normal, lightDir));
  if (intensity>0) intensity *= 1.0 - shadow();
  return u_lightColor * intensity;
}

/* returns a vector that is orthogonal to v */
vec3 ortho(vec3 v) {
  vec3 w = abs(dot(v, normalize(vec3(.1,0,1)))) < 0.999 
    ? vec3(.1,0,1) : vec3(1,0,0);
  return normalize(cross(v, w));
}

/* returns true if pos is inside the unit cube */
bool insideVoxel(vec3 pos) {
  return all(lessThanEqual(pos, vec3(1.0f))) 
      && all(greaterThanEqual(pos, vec3(0.0f)));
}

/* convert the range [-1, +1] to [0, +1] */
vec3 toVoxel(vec3 pos) {
  return pos*(0.5)+0.5;  
}

/* returns a pseudo random float using v as seed */
float random(vec3 v) {
  float a = dot(v, vec3(29.29384, 62.29384, 48.23478));
  a = sin(a);
  a = a * 293487.1273498;
  a = fract(a);
  return a;
}

/* returns a preudo random vec3 using v as seed */
vec3 random3(vec3 v){
  v = fract(v * vec3(443.897, 441.423, 437.195));
  v += dot(v, v.zxy + vec3(31.31));
  return fract((v.xyz + v.yzx) * v.zyx);
}

/* converts a 32 bit rgba value to a vec4 [0.0, 255.0]*/
vec4 rgbaToVec(uint val) {
  return vec4(
      float((val&0x000000FFu)), 
      float((val&0x0000FF00u)>>8U), 
      float((val&0x00FF0000u)>>16U), 
      float((val&0xFF000000u)>>24U)
      );
}

/* converts a vec4 to a 32 bit rgba */
uint vecToRgba(vec4 val) {
  vec4 v = clamp(val, vec4(0), vec4(255));
  return (
      (uint(v.w)&0x000000FFu)<<24U |
      (uint(v.z)&0x000000FFu)<<16U |
      (uint(v.y)&0x000000FFu)<<8U  |
      (uint(v.x)&0x000000FFu)
      );
}

/* performs atomic average image store */
/* source: https://www.seas.upenn.edu/~pcozzi/OpenGLInsights/OpenGLInsights-SparseVoxelization.pdf 
 * */
void imageAtomicAvg(image3D imgUI, ivec3 coords, vec4 val) {

  /* not compiling on some drivers
     val.xyz = val.xyz * 255.f;
     uint newVal = vecToRgba(val);
     uint prevVal = 0;
     uint curVal;

     while (true) {
     uint curVal = imageAtomicCompSwap(imgUI, coords, prevVal, newVal);
     if (curVal == prevVal) return;
     prevVal = curVal;
     vec4 rval = rgbaToVec(curVal);
     rval.xyz = (rval.xyz * rval.w);
     vec4 curValF = rval + val;
     curValF.xyz /= curValF.w;
     newVal = vecToRgba(curValF);
     }
   */
}

/* performs cone tracing starting at from and going into direction dir with
   cone aperture of apertureTan2, which is = 2*tan(aperture/2) */
vec3 ConeTracing(vec3 from, vec3 dir, float apertureTan2) {
  vec4 color = vec4(0); /* starting color and opacity */
  float dist = voxelSize * 0; /* starting distance */

  /* makes sure the aperture is > 0 */
  float a2 = max(voxelSize, apertureTan2);

  /* keep looping until the cone is fully occluded */
  while (color.a < 1.0) {
    /* the diameter of the cone at distance dist */
    float diameter = max(voxelSize, dist * a2);
    float lod = log2(diameter * u_voxelCount); 
    vec3 pos = from + dir * dist; /* updates current position */
    if (!insideVoxel(pos)) break; /* returns if outside the 3D texture */
    vec4 cc = fetch(dir, pos, lod);
    color += (1 - color.a) * cc; /* scale by unoccluded area */

    dist += diameter * 0.7; /* march along the ray direction */
  }

  return color.rgb;
}

/* Casts some ray to approximate indirect diffuse lighting at the current pos */
vec3 TraceDiffuse(vec3 normal) {
  vec3 pos = toVoxel(b_pos); /* normalize pos to texture coordinates */
  pos += 8 * voxelSize * normal; /* offset to prevent self lighting */

 /* number of cones to use */
#define CONES 5
  const vec3 cones[9] = vec3[9]( 
      vec3(0,0,1), vec3(1,0,1), vec3(-1,0,1), vec3(0,1,1), vec3(0,-1,1), 
      vec3(1,1,1), vec3(1,-1,1), vec3(-1,1,1), vec3(-1,-1,1));

  float apertureAngle = 30; /* the aperture must be less than 45 degrees */
  float apertureTan2 = 2*tan(radians(apertureAngle)/2); 

  vec3 xtan, ytan; 
  if(true) { /* random cone directions */
    xtan = normalize(random3(pos+time) * 2 + 1);
    xtan = normalize(random3(pos+time));
    if (abs(dot(xtan, normal)) > 0.999) {
      xtan = ortho(normal);
    }
    ytan = cross(normal, xtan);
    xtan = cross(normal, ytan);
  } else { /* not random */
    xtan = ortho(normal);
    ytan = cross(normal, xtan);
  }

  /* normal tangent space */
  mat3 tanSpace = mat3(xtan, ytan, normal); 

  vec3 color = vec3(0);
  for (int i = 0; i < CONES; i++) {
    vec3 dir = tanSpace * cones[i];
    color += ConeTracing(pos, normalize(dir), apertureTan2);
  }
  return color; /* the returning color is the sum of all casted cones */
}

void main(){

  vec3 posVoxelSpace = toVoxel(b_pos) * 0.999999f; /* needed to include 1.0 */
  ivec3 dim = imageSize(u_voxelTexture);

  /* color correction gamma exponents */
  float gammaEditor = 2.2;
  float gammaTextures = 2.2;

  if (u_particle) { /* if we are drawing fire, we need to add colors not avg */
    vec3 pos = b_pos;
    vec3 emission = pow(u_emission, vec3(gammaEditor));
    float opacity = texture(u_emissionMap, b_uv).r * u_opacity;

    vec4 color = vec4(emission * opacity, 0.0);

    /* no atomic add, but there are no visual artifacts */
    vec4 prev = imageLoad(u_voxelTexture, ivec3(dim*posVoxelSpace));
    color += prev;
    imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), color);
    return;
  }

  vec3 pos = b_pos;
  vec3 normal = b_normal;

  vec3 albedo = pow(u_diffuse, vec3(gammaEditor));
  vec3 emission = pow(u_emission, vec3(gammaEditor));
  emission *= u_emissionScale;

  float rougness = u_rough;
  float metalness = u_metal;

  if (u_useMaps) { /* if we are using textures sample them */
    albedo *= pow(texture(u_diffuseMap, b_uv).rgb, vec3(gammaTextures));
    emission *= pow(texture(u_emissionMap, b_uv).rgb, vec3(gammaTextures));

    /* normalize inside fragment shader because of 
     * interpolation between vertices*/
    mat3 tanSpace = mat3(
        normalize(b_xtan), normalize(b_ytan), normalize(b_normal));
    vec2 tn = texture(u_bumpMap, b_uv).rg * 2 - 1; /*remaps normal to [-1,+1] */
    normal = vec3(tn, sqrt(1 - dot(tn, tn))); /* calculates z component */
    normal = tanSpace * normal;	/* normal vector corrected with normal map */
  }
  normal = normalize(normal);

  /* direct lighting */
  vec3 color = albedo * spotlight(normal) / PI;

  /* indirect lighting */
  if (u_temporalMultibounce) {
    vec3 indirect = TraceDiffuse(normal);
    indirect *= albedo;
    color += indirect * u_restitution;
  }

  /* emission */
  color += emission;

  vec4 val = vec4(color, 1.0f);

  if (u_averageValues) {
    imageAtomicAvg(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
  } else {
    /* we still try to average even without atomic operations, reduces flicker
     * by a bit */
    val.a *= 1.f / 255.f;
    vec4 prev = imageLoad(u_voxelTexture, ivec3(dim*posVoxelSpace));
    vec3 avg = (prev.rgb * prev.a + val.rgb * val.a) / (prev.a + val.a);
    val = vec4(avg, prev.a + val.a);
    imageStore(u_voxelTexture, ivec3(dim*posVoxelSpace), val);
  }
}

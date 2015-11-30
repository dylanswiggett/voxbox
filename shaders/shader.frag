#version 430 core
#extension GL_NV_gpu_shader5 : require

layout(location = 0) out vec3 color;

// TODO: Probably have optionally 8 separate illumination values (1 for each side)
struct voxel_data {
  uint8_t r, g, b;
  uint8_t emission;
  uint16_t illum_r, illum_g, illum_b;
  uint16_t numrays;
  uint8_t diffuse;
  uint8_t neighbors;
  uint16_t flags;
  uint lock;
};

layout (std430, binding=1) buffer voxel_buf {
  voxel_data vdata[];
};

vec3 vd_color(struct voxel_data vd) {
  return vec3(ivec3(vd.r,vd.g,vd.b)) / 255;
}

uniform isampler3D voxels;
uniform ivec2 wsize;
uniform vec2 viewoff;
uniform vec3 corner, dim;
uniform ivec3 nvoxels;
uniform int time;
uniform int update;

uniform ivec3 ray[400];
uniform isampler2D rays;
uniform int raylen, numrays;
uniform vec3 raydir;

vec3 c1, c2;

// Returns:
//     1 for voxel
//     0 for no-hit
//    -1 for outside
int voxelAt(ivec3 pos, out int vloc) {
  if (pos.y < 0 || pos.y >= nvoxels.y)
    return -1;
  
  pos.x %= nvoxels.x;
  pos.z %= nvoxels.z;
  /*
  vec3 s = step(vec3(0,0,0), pos) - step(nvoxels, pos);
  if (s.x * s.y * s.z == 0)
    return -1;
  */

  vloc = texelFetch(voxels, pos, 0).r;
  if (vloc > 0) {
    return 1;
  }

  vloc = -vloc; // vloc is now the distance;
  return 0;
}

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

ivec3 isoraymarch(vec3 pos, out int vloc, out ivec3 laststep) {
  vec3 voxelScale = (dim - corner) / nvoxels;
  vec3 offset = pos - corner;
  ivec3 curVoxel = ivec3(floor(offset / voxelScale));
  vec3 curVoxelOffset = offset - voxelScale * curVoxel;
  
  int rot1, rot2;

  ivec3 s;
  
  float x = curVoxelOffset.x;
  float y = curVoxelOffset.y;
  float z = curVoxelOffset.z;

  ivec3 steps[5];

  if (x > y)
    if (x > z) {
      if (y > z) { // z, y, x
	s = ivec3(-1,0,0);
	rot1 = 0; rot2 = 1;
      } else { // y, z, x
	s = ivec3(-1,0,0);
	rot1 = 1; rot2 = 0;
      }
    } else { // y, x, z
      s = ivec3(0,0,-1);
      rot1 = 0; rot2 = 1;
    }
  else
    if (y > z) {
      if (x > z) { // z, x, y
	s = ivec3(0,-1,0);
	rot1 = 1; rot2 = 0;
      } else { // x, z, y
	s = ivec3(0,-1,0);
	rot1 = 0; rot2 = 1;
      }
    } else { // x, y, z
      s = ivec3(0,0,-1);
      rot1 = 1; rot2 = 0;
    }

  steps[0] = steps[3] = s;
  s = s.zxy * rot1 + s.yzx * rot2;
  steps[1] = steps[4] = s;
  s = s.zxy * rot1 + s.yzx * rot2;
  steps[2] = s;

  int idx = 0;

  int stat = voxelAt(curVoxel, vloc);
  laststep = steps[0];
  if (stat == 1) {
    vloc = 0;
    return ivec3(-1,-1,-1);
  }

  idx = 1;
  ivec3 off = ivec3(viewoff.x / voxelScale.x, 0, viewoff.y / voxelScale.y);

  while (curVoxel - off == abs(curVoxel - off) && stat == 0) {
    curVoxel += steps[idx] * ((vloc + 2) / 3) + steps[idx + 1] * ((vloc + 1) / 3) +
      steps[idx + 2] * (vloc / 3);
    idx = (idx + vloc) % 3;
    stat = voxelAt(curVoxel, vloc);
  }
  if (stat == 1) {
    laststep = steps[idx + 2];
    return curVoxel;
  }
  return ivec3(-1,-1,-1);
}

bool raymarch(ivec3 pos, ivec3 norm, out int vloc, out vec3 randdir) {
  int r = int(numrays * rand(vec2(pos.x ^ pos.y, pos.y ^ pos.z)));
  int stat;
  int i = 0;
  ivec3 off;
  vloc = 1;
  while (i < raylen && stat == 0) {
    off = texelFetch(rays, ivec2(i, (time + r) % numrays), 0).rgb;
    // More realistic lighting, but at a perf hit.
    off = off * (ivec3(1,1,1) - abs(norm)) + abs(off) * norm;
    ivec3 offpos = pos + off;
    i += vloc;
    stat = voxelAt(offpos, vloc);
  }
  randdir = normalize(off);
  if (stat == 0)
    return false;
  else if (stat == 1)
    return true;
}

float intersection(vec3 pos, vec3 dir) {
  // TODO: Since the view is isometric,
  //       this can be much more efficient!
  vec3 t1 = (c1 - pos) / dir;
  vec3 t2 = (c2 - pos) / dir;
  vec3 tmins = min(t1, t2);
  vec3 tmaxs = max(t1, t2);
  float tmin = max(tmins.x, max(tmins.y, tmins.z));
  float tmax = min(tmaxs.x, min(tmaxs.y, tmaxs.z));
  return tmin * step(tmin, tmax);
}

void process_voxel(inout voxel_data vox, ivec3 laststep, ivec3 voxel) {
  // Requires a lock be held on vox.
  
  // We might not use this if we don't get the lock, but generating it
  // is cheap enough that's it worth it for faster convergence.
  ivec3 lightpos = voxel - laststep;
  int nextvloc;
  vec3 randdir;
  vec3 lighting = vec3(0,0,0);
  if (raymarch(lightpos, -laststep, nextvloc, randdir)) {
    voxel_data hitvox = vdata[nextvloc];
    vec3 hit_color = vd_color(hitvox);
    vec3 hit_illum = vec3(ivec3(hitvox.illum_r,
				hitvox.illum_g,
				hitvox.illum_b)) / (10.0 * float(hitvox.numrays));
    float hit_emit = float(hitvox.emission) / 10; // Small lights can generate LOTS of light!
    float hit_diffuse = float(hitvox.diffuse) / 255; // But reflecting cannot amplify.
    vec3 direct_lighting = hit_color * hit_emit;
    vec3 indirect_lighting = hit_color * hit_illum * hit_diffuse;
    lighting = direct_lighting + indirect_lighting; //max(direct_lighting, indirect_lighting);
  } else {
    lighting = vec3(1,1,1) * dot(randdir, vec3(1, .2, -.3));
  }


  // TODO: Remove magic numbers.
  int maxsample = 2000;
  int minussample = 500;
  if (vox.numrays >= uint16_t(maxsample)) {
    float downscale = float(maxsample - minussample) / maxsample;
    vox.numrays -= uint16_t(minussample);
    vox.illum_r = uint16_t(float(vox.illum_r) * downscale);
    vox.illum_g = uint16_t(float(vox.illum_g) * downscale);
    vox.illum_b = uint16_t(float(vox.illum_b) * downscale);
  } else {
    vox.numrays++;
    vox.illum_r += uint16_t(10 * lighting.r);
    vox.illum_g += uint16_t(10 * lighting.g);
    vox.illum_b += uint16_t(10 * lighting.b);
    if (vox.illum_r > vox.numrays * uint16_t(10) ||
	vox.illum_g > vox.numrays * uint16_t(10) ||
	vox.illum_b > vox.numrays * uint16_t(10)) {
      float minscale = float(vox.numrays) * 10 /
	max(float(vox.illum_r), max(float(vox.illum_g), float(vox.illum_b)));
      vox.illum_r = uint16_t(float(vox.illum_r) * minscale);
      vox.illum_g = uint16_t(float(vox.illum_g) * minscale);
      vox.illum_b = uint16_t(float(vox.illum_b) * minscale);
    }
  }
}

void main() {
  float mindim = min(wsize.x, wsize.y);
  vec2 pos = (2 * (gl_FragCoord.xy - wsize / 2) / mindim);
  c1 = corner;
  c2 = corner + dim;
  
  vec3 cameradir = normalize(vec3(-1,-1,-1));
  vec3 right = normalize(cross(cameradir, vec3(0,1,0)));
  vec3 up = cross(right, cameradir);
  vec3 camerapos = corner + dim + (right * pos.x + up * pos.y) * length(dim) / 2;

  float t = intersection(camerapos, cameradir);
  if (t == 0) {
    color = vec3(.1, .1, .2);
    return;
  }
  camerapos += cameradir * (t + .01) + vec3(viewoff.x, 0, viewoff.y);

  int vloc;
  ivec3 laststep;
  ivec3 voxel = isoraymarch(camerapos, vloc, laststep);
  if (voxel.x < 0) {
    if (vloc == 0)
      color = vec3(.15, .15, .3);
    else
      color = vec3(.2, .2, .4);
    return;
  }

  int check = time;
  if (atomicExchange(vdata[vloc].lock, check) != check) {
    // Begin locked region.
    process_voxel(vdata[vloc], laststep, voxel);
    // End locked region.
  }

  // Nice and racey
  color = vd_color(vdata[vloc]) *
    (vec3(1,1,1) * vdata[vloc].emission / 10 +
     vec3(vdata[vloc].illum_r, vdata[vloc].illum_g, vdata[vloc].illum_b) /
     (10.0 * float(vdata[vloc].numrays)));
  color = clamp(color, 0, 1);

  return;
}

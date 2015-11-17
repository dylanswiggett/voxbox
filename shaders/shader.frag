#version 430 core
#extension GL_NV_gpu_shader5 : require

layout(location = 0) out vec3 color;

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

// These don't work :(
/*
bool lock_voxel(uint vloc) {
  int maxtries = 1000;
  while (maxtries > 0) {
    maxtries--;
    if (atomicExchange(vdata[vloc].lock, 1) == 0)
      return true;
  }
  return false;
}

void unlock_voxel(uint vloc) {
  memoryBarrier();
  atomicExchange(vdata[vloc].lock, 0);
}
*/

uniform usampler3D voxels;

uniform ivec2 wsize;

uniform vec3 corner, dim;
uniform ivec3 nvoxels;

uniform int time;

vec3 c1, c2;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

vec3 hem_rand(vec3 norm, vec3 side, vec3 seed) {
  float u1 = rand(vec2(rand(vec2(seed.x, seed.y)), rand(vec2(seed.z, time))));
  float u2 = rand(vec2(u1, time));
  float r = sqrt(1.0 - u1 * u1);
  float phi = 2 * 3.14159 * u2;
  vec3 side2 = cross(norm, side);
  return u1 * norm + cos(phi) * r * side + sin(phi) * r * side2;
}

ivec3 raymarch(vec3 pos, vec3 dir, out uint vloc, out ivec3 laststep) {
  vec3 voxelScale = (dim - corner) / nvoxels;
  ivec3 steps = ivec3(sign(dir));

  // TODO: Can maxs be found more efficiently?
  vec3 offset = pos - corner;
  ivec3 curVoxel = ivec3(floor(offset / voxelScale));
  if (curVoxel.x < 0 || curVoxel.x >= nvoxels.x ||
      curVoxel.y < 0 || curVoxel.y >= nvoxels.y ||
      curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
    return ivec3(-1, -1, -1); // Edge case (hurr durr)
  vec3 curVoxelOffset = offset - voxelScale * curVoxel;
  vec3 maxs = max(curVoxelOffset * steps, (curVoxelOffset - voxelScale) * steps) / dir;
  
  vec3 deltas = abs(voxelScale / dir);
  
  int maxtest = 1000; // Just to avoid infinite loops :)
  while (maxtest > 0) {
    // TODO: Optimize these divisions!
    maxtest -= 1;

    // Are we in a voxel?
    vloc = texture(voxels, vec3(curVoxel) / nvoxels).r;
    if (vloc != 0) {
      vloc -= 1;
      return curVoxel;
    }

    if (maxs.x < maxs.y) {
      if (maxs.x < maxs.z) {
	curVoxel.x += steps.x;
	laststep = ivec3(steps.x, 0, 0);
	if (curVoxel.x < 0 || curVoxel.x >= nvoxels.x)
	  break;
	maxs.x += deltas.x;
      } else {
	curVoxel.z += steps.z;
  	laststep = ivec3(0, 0, steps.z);
	if (curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
	  break;
	maxs.z += deltas.z;
      }
    } else {
      if (maxs.y < maxs.z) {
	curVoxel.y += steps.y;
	laststep = ivec3(0, steps.y, 0);
	if (curVoxel.y < 0 || curVoxel.y >= nvoxels.y)
	  break;
	maxs.y += deltas.y;
      } else {
	curVoxel.z += steps.z;
	laststep = ivec3(0, 0, steps.z);
	if (curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
	  break;
	maxs.z += deltas.z;
      }
    }
  }
  return ivec3(-1, -1, -1);
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
  if (tmax >= tmin) {
    return tmin;
  } else {
    return 0.0;
  }
}

void main() {
  float mindim = min(wsize.x, wsize.y);
  vec2 pos = (2 * (gl_FragCoord.xy - wsize / 2) / mindim);
  c1 = corner;
  c2 = corner + dim;
  
  vec3 cameradir = -normalize(dim);
  vec3 right = normalize(cross(cameradir, vec3(0,1,0)));
  vec3 up = cross(right, cameradir);
  // TODO: Figure out why 25 is a good parameter here...
  vec3 camerapos = corner + dim + (right * pos.x + up * pos.y) * 25;

  float tscaled = float(time) / 100;
  float s = sin(tscaled);
  float c = cos (tscaled);
  
  // This line enables (crazy) perspective:
  //cameradir = normalize(-dim + 20 * sin(float(time) / 10) * (right * pos.x + up * pos.y));
  //cameradir = normalize(-dim + 20 * (right * pos.x + up * pos.y));

  float t = intersection(camerapos, cameradir);
  if (t == 0) {
    color = vec3(.1, .1, .2);
    return;
  }

  camerapos += cameradir * (t + .1);

  uint vloc;
  ivec3 laststep;
  ivec3 voxel = raymarch(camerapos, cameradir, vloc, laststep);
  if (voxel.x < 0) {
    color = vec3(.2, .2, .4);
    return;
  }

  ivec3 lightpos = voxel - laststep;
  vec3 norm = -vec3(laststep);
  vec3 side = vec3(norm.y, norm.z, norm.x);
  vec3 lightdir = normalize(hem_rand(norm, side, vec3(voxel)));
  vec3 lightposabs = corner + (dim / vec3(nvoxels)) * (vec3(lightpos) + vec3(.5,.5,.5));

  ivec3 nextlaststep;
  uint nextvloc;
  voxel = raymarch(lightposabs, lightdir, nextvloc, nextlaststep);

  float lighting = 0;
  vec3 ldir = vec3(1, .1, -1);
  /*
  if (voxel.x < 0 && dot(lightdir, ldir) > 0) {
    lighting = dot(lightdir, ldir);
  }
  */
  if (vdata[vloc].b > vdata[vloc].g) {
    lighting = float(vdata[vloc].b) / 255;
  }
  if (voxel.x > 0) {
    voxel_data hit = vdata[nextvloc];
    if (hit.b > hit.g)
      lighting += float(hit.b) / 20;
  }

  float actuallight = 0;

  // Lock vdata for our voxel.
  bool done = false;
  uint locked = 0;
  while (!done) {
    locked = atomicExchange(vdata[vloc].lock, 1);
    if (locked == 0) {
      // Begin locked region.

      if (vdata[vloc].numrays >= uint16_t(5000)) {
	vdata[vloc].numrays -= uint16_t(2000);
	vdata[vloc].illum_r = uint16_t(float(vdata[vloc].illum_r) * 3.0 / 5.0);
      } else {
	vdata[vloc].numrays++;
	vdata[vloc].illum_r += uint16_t(10 * lighting);
	if (vdata[vloc].illum_r > vdata[vloc].numrays * uint16_t(10))
	  vdata[vloc].illum_r = vdata[vloc].numrays * uint16_t(10);
      }

      actuallight = float(vdata[vloc].illum_r) / float(vdata[vloc].numrays) / 10;
      actuallight = clamp(actuallight, 0, 1);

      // End locked region.
      memoryBarrier();
      atomicExchange(vdata[vloc].lock, 0);
      done = true;
    }
  }

  /*
  atomicAdd(vdata[vloc].metadata, 10);
  vec3 ldir = vec3(1,.5,-.2);
  if (voxel.x < 0 && dot(lightdir, ldir) > 0) {
    atomicAdd(vdata[vloc].lighting, int(10 * dot(lightdir, ldir)));
  }
  */

  color = vec3(vd_color(vdata[vloc])) * actuallight;
  return;
}

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

uniform usampler2D rays;
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
  // This distribution seems good enough.
  float u1 = rand(vec2(time - seed.x, time + seed.y));
  float u2 = rand(vec2(time - seed.x, time + seed.z));
  float r = sqrt(1.0 - u1 * u1);
  float phi = 2 * 3.14159 * u2;
  vec3 side2 = normalize(cross(norm, side));
  return u1 * norm + cos(phi) * r * side + sin(phi) * r * side2;
}

// TODO: Rays can be generated in advance! Store as a tex.
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

  vec3 nvoxelrecip = vec3(1,1,1) / nvoxels;
  
  int maxtest = 1000; // Just to avoid infinite loops :)
  while (maxtest > 0) {
    // TODO: Optimize these divisions!
    maxtest -= 1;

    // Are we in a voxel?
    vloc = texture(voxels, vec3(curVoxel) * nvoxelrecip).r;
    if (vloc != 0) {
      vloc -= 1;
      return curVoxel;
    }

    if (maxs.x < maxs.y) {
      if (maxs.x < maxs.z) {
	curVoxel.x += steps.x;
	laststep = ivec3(steps.x, 0, 0);
	if (curVoxel.x < 0 || curVoxel.x >= nvoxels.x)
	  return ivec3(-1,-1,-1);
	maxs.x += deltas.x;
      } else {
	curVoxel.z += steps.z;
  	laststep = ivec3(0, 0, steps.z);
	if (curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
	  return ivec3(-1,-1,-1);
	maxs.z += deltas.z;
      }
    } else {
      if (maxs.y < maxs.z) {
	curVoxel.y += steps.y;
	laststep = ivec3(0, steps.y, 0);
	if (curVoxel.y < 0 || curVoxel.y >= nvoxels.y)
	  return ivec3(-1,-1,-1);
	maxs.y += deltas.y;
      } else {
	curVoxel.z += steps.z;
	laststep = ivec3(0, 0, steps.z);
	if (curVoxel.z < 0 || curVoxel.z >= nvoxels.z)
	  return ivec3(-1,-1,-1);
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
  vec3 camerapos = corner + dim + (right * pos.x + up * pos.y) * length(dim) / 2;

  float tscaled = float(time) / 100;
  float s = sin(tscaled);
  float c = cos (tscaled);
  
  // This line enables (crazy) perspective:
  //cameradir = normalize(-dim + 20 * sin(float(time) / 10) * (right * pos.x + up * pos.y));
  //cameradir = normalize(-dim + 10 * (right * pos.x + up * pos.y));

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

  // We might not use this if we don't get the lock, but generating it
  // is cheap enough that's it worth it for faster convergence.
  ivec3 lightpos = voxel - laststep;
  vec3 norm = -vec3(laststep);
  vec3 side = vec3(norm.y, norm.z, norm.x);
  vec3 lightdir = normalize(hem_rand(norm, side, vec3(voxel.x, pos.x, pos.y)));
  vec3 lightposabs = corner + (dim / vec3(nvoxels)) * (vec3(lightpos) + .5);

  ivec3 nextlaststep;
  uint nextvloc;
  voxel = raymarch(lightposabs, lightdir, nextvloc, nextlaststep);

  vec3 lighting = vec3(0,0,0);
  if (voxel.x >= 0) {
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
    //lighting = vec3(1,1,1) * dot(lightdir, normalize(vec3(1, .2, -.3)));
  }

  // Lock vdata for our voxel.
  int triesleft = 5; // Don't block for too long.
  uint locked = 0;
  while (triesleft > 0) {
    triesleft--;
    locked = atomicExchange(vdata[vloc].lock, 1);
    if (locked == 0) {
      // Begin locked region.

      // TODO: Remove magic numbers.
      int maxsample = 5000;
      int minussample = 2000;
      float downscale = float(maxsample - minussample) / maxsample;
      if (vdata[vloc].numrays >= uint16_t(maxsample)) {
	vdata[vloc].numrays -= uint16_t(minussample);
	vdata[vloc].illum_r = uint16_t(float(vdata[vloc].illum_r) * downscale);
	vdata[vloc].illum_g = uint16_t(float(vdata[vloc].illum_g) * downscale);
	vdata[vloc].illum_b = uint16_t(float(vdata[vloc].illum_b) * downscale);
      } else {
	vdata[vloc].numrays++;
	vdata[vloc].illum_r += uint16_t(10 * lighting.r);
      	vdata[vloc].illum_g += uint16_t(10 * lighting.g);
	vdata[vloc].illum_b += uint16_t(10 * lighting.b);
	if (vdata[vloc].illum_r > vdata[vloc].numrays * uint16_t(10) ||
	    vdata[vloc].illum_g > vdata[vloc].numrays * uint16_t(10) ||
	    vdata[vloc].illum_b > vdata[vloc].numrays * uint16_t(10)) {
	  float minscale =
	    min(float(vdata[vloc].numrays) * 10 / float(vdata[vloc].illum_r),
		min(float(vdata[vloc].numrays) * 10 / float(vdata[vloc].illum_g),
		    float(vdata[vloc].numrays) * 10 / float(vdata[vloc].illum_b)));
	  vdata[vloc].illum_r = uint16_t(float(vdata[vloc].illum_r) * minscale);
	  vdata[vloc].illum_g = uint16_t(float(vdata[vloc].illum_g) * minscale);
	  vdata[vloc].illum_b = uint16_t(float(vdata[vloc].illum_b) * minscale);
	}
      }

      // End locked region.
      memoryBarrier();
      atomicExchange(vdata[vloc].lock, 0);
      triesleft = 0;
    }
  }

  // Ooooh, racey!
  color = vd_color(vdata[vloc]) *
    (vec3(1,1,1) * vdata[vloc].emission / 10 +
     vec3(vdata[vloc].illum_r, vdata[vloc].illum_g, vdata[vloc].illum_b) /
     (10.0 * float(vdata[vloc].numrays)));
  color = clamp(color, 0, 1);
  return;
}

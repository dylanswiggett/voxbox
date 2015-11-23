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

uniform usampler3D voxels;
uniform ivec2 wsize;
uniform vec2 viewoff;
uniform vec3 corner, dim;
uniform ivec3 nvoxels;
uniform int time;
uniform int update;

uniform ivec3 ray[400];
uniform vec3 raydir;

vec3 c1, c2;

float rand(vec2 co){
  return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

ivec3 isoraymarch(vec3 pos, out uint vloc, out ivec3 laststep) {
  vec3 voxelScale = (dim - corner) / nvoxels;
  vec3 offset = pos - corner;
  ivec3 curVoxel = ivec3(floor(offset / voxelScale));
  vec3 curVoxelOffset = offset - voxelScale * curVoxel;
  
  ivec3 stepx = ivec3(-1,0,0);
  ivec3 stepy = ivec3(0,-1,0);
  ivec3 stepz = ivec3(0,0,-1);
  ivec3 step1, step2, step3;
  
  float x = curVoxelOffset.x;
  float y = curVoxelOffset.y;
  float z = curVoxelOffset.z;
  
  if (x > y)
    if (x > z) {
      if (y > z) { // z, y, x
	step1 = stepz; step2 = stepy; step3 = stepx;
      } else { // y, z, x
	step1 = stepy; step2 = stepz; step3 = stepx;
      }
    } else { // y, x, z
      step1 = stepy; step2 = stepx; step3 = stepz;
    }
  else
    if (y > z) {
      if (x > z) { // z, x, y
	step1 = stepz; step2 = stepx; step3 = stepy;
      } else { // x, z, y
	step1 = stepx; step2 = stepz; step3 = stepy;
      }
    } else { // x, y, z
      step1 = stepx; step2 = stepy; step3 = stepz;
    }

  while (true) {
    vloc = texelFetch(voxels, curVoxel, 0).r;
    if (vloc != 0) {
      vloc -= 1; laststep = step3;
      return curVoxel;
    }
    
    curVoxel += step1;
    if (dot(curVoxel, step1) > 0)
      break;
    vloc = texelFetch(voxels, curVoxel, 0).r;
    if (vloc != 0) {
      vloc -= 1; laststep = step1;
      return curVoxel;
    }
    
    curVoxel += step2;
    if (dot(curVoxel, step2) > 0)
      break;
    vloc = texelFetch(voxels, curVoxel, 0).r;
    if (vloc != 0) {
      vloc -= 1; laststep = step2;
      return curVoxel;
    }
    
    curVoxel += step3;
    if (dot(curVoxel, step3) > 0)
      break;
  }
  return ivec3(-1,-1,-1);
}

bool raymarch(ivec3 pos, ivec3 norm, out uint vloc) {
  int i = 0;
  ivec3 scale = ivec3(1,1,1);
  int r1 = 1 - 2 * int(2 * rand(vec2(pos.x & pos.y, time)));
  int r2 = 1 - 2 * int(2 * rand(vec2(pos.y & pos.z, time)));

  if (norm.x != 0) {
    scale = ivec3(norm.x, r1, r2);
  } else if (norm.y != 0) {
    scale = ivec3(r1, norm.y, r2);
  } else {
    scale = ivec3(r1, r2, norm.z);
  }

  while (true) {
    ivec3 offpos = pos + ray[i] * scale;

    /*
    ivec3 steps = ivec3(step(ivec3(0,0,0), offpos) - step(nvoxels, offpos));
    if (steps.x + steps.y + steps.z != 3)
      return false;
    */
    if (offpos.x < 0 || offpos.x >= nvoxels.x ||
	offpos.y < 0 || offpos.y >= nvoxels.y ||
	offpos.z < 0 || offpos.z >= nvoxels.z)
      return false;

    vloc = texelFetch(voxels, offpos, 0).r;
    if (vloc != 0) {
      vloc -= 1;
      return true;
    }

    i++;
  }
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

  camerapos += cameradir * t + vec3(viewoff.x, 0, viewoff.y);

  uint vloc;
  ivec3 laststep;
  ivec3 voxel = isoraymarch(camerapos, vloc, laststep);
  if (voxel.x < 0) {
    color = vec3(.2, .2, .4);
    return;
  }

  // Lock vdata for our voxel.
  uint locked = 0;
  if (update == 1 && vdata[vloc].flags != uint16_t(time % 10) && vdata[vloc].lock == 0)
    locked = atomicExchange(vdata[vloc].lock, 1);
  else
    locked = 1;
  if (locked == 0) {
    // Begin locked region.

    // We might not use this if we don't get the lock, but generating it
    // is cheap enough that's it worth it for faster convergence.
    ivec3 lightpos = voxel - laststep;
    uint nextvloc;
    vec3 lighting = vec3(0,0,0);
    if (raymarch(lightpos, -laststep, nextvloc)) {
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
      //lighting = vec3(1,1,1) * dot(raydir, vec3(1, .2, -.3));
    }


    // TODO: Remove magic numbers.
    int maxsample = 5000;
    int minussample = 2000;
    if (vdata[vloc].numrays >= uint16_t(maxsample)) {
      float downscale = float(maxsample - minussample) / maxsample;
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
	float minscale = float(vdata[vloc].numrays) * 10 /
	  max(float(vdata[vloc].illum_r), max(float(vdata[vloc].illum_g), float(vdata[vloc].illum_b)));
	vdata[vloc].illum_r = uint16_t(float(vdata[vloc].illum_r) * minscale);
	vdata[vloc].illum_g = uint16_t(float(vdata[vloc].illum_g) * minscale);
	vdata[vloc].illum_b = uint16_t(float(vdata[vloc].illum_b) * minscale);
      }
    }

    vdata[vloc].flags = uint16_t(time % 10);

    // End locked region.
    // memoryBarrier();
    atomicExchange(vdata[vloc].lock, 0);
  } else if (locked == 0) {
    atomicExchange(vdata[vloc].lock, 0);
  }

  // Ooooh, racey!
  color = vd_color(vdata[vloc]) *
    (vec3(1,1,1) * vdata[vloc].emission / 10 +
     vec3(vdata[vloc].illum_r, vdata[vloc].illum_g, vdata[vloc].illum_b) /
     (10.0 * float(vdata[vloc].numrays)));
  color = clamp(color, 0, 1);
  return;
}

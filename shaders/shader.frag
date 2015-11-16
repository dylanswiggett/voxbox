#version 430 core

layout(location = 0) out vec3 color;

struct voxel_data {
  uint color; // (r g b emittance)
  uint lighting; // (r g b diffuse)
  uint metadata; // (numrays neighbors flags flags)
};

vec3 vd_color(struct voxel_data vd) {
  uint b = (vd.color >> 16) & 0xFF;
  uint g = (vd.color >> 8) & 0xFF;
  uint r = (vd.color >> 0) & 0xFF;
  return vec3(ivec3(r,g,b)) / 255;
}

uniform usampler3D voxels;

uniform ivec2 wsize;

uniform vec3 corner, dim;
uniform ivec3 nvoxels;

uniform int time;

vec3 c1, c2;

layout (std430, binding=1) buffer voxel_buf {
  voxel_data vdata[];
};

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
  
  // This line enables (crazy) perspective:
  // cameradir = normalize(-dim + 20 * sin(float(time) / 10) * (right * pos.x + up * pos.y));
  // cameradir = normalize(-dim + 20 * (right * pos.x + up * pos.y));

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
  atomicAdd(vdata[vloc].metadata, 10);
  vec3 ldir = vec3(1,.5,-.2);
  if (voxel.x < 0 && dot(lightdir, ldir) > 0) {
    atomicAdd(vdata[vloc].lighting, int(10 * dot(lightdir, ldir)));
  }

  color = vec3(vd_color(vdata[vloc])) * (float(vdata[vloc].lighting) / float(vdata[vloc].metadata));
  return;
}

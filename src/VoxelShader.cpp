#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <algorithm>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

#include "VoxelShader.hpp"
#include "VoxelData.h"
#include "shader_loader.h"

#define RAYLEN 200
#define NUMRAYS 1000

#define PI 3.141592

using namespace glm;

float plane[] = {
  -1, -1, 0, // 1----2
  -1,  1, 0, // |  / | Is it a bird?
   1,  1, 0, // | /  | Is it a plane?
   1, -1, 0  // 0----3
};

int plane_indices[] = {0,1,2,0,2,3};

void check_GLerror()
{
  GLenum err = glGetError();
  if (err != GL_NO_ERROR) {
    cout << "ERROR in OpenGL: " << err << endl;
    exit(0);
  }
}

VoxelShader::VoxelShader(VoxelData* data,
			 float x, float y, float z,
			 float w, float h, float d,
			 int nx, int ny, int nz) :
  data_(data), x_(x), y_(y), z_(z),
  w_(w), h_(h), d_(d), nx_(nx), ny_(ny), nz_(nz)
{
  glEnable(GL_TEXTURE_3D);
  glEnable(GL_TEXTURE_2D);

  // Initialize chunk map.
  num_chunks_wide_ = 1 / CHUNK_DIM + 2 * BUFFER;
  for (int i = 0; i < num_chunks_wide_ * num_chunks_wide_; i++)
    chunk_contents_.push_back(NULL_CHUNK);
  
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  
  // Set up basic rendering plane
  glGenBuffers(1, &vertex_buffer_);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
  glGenBuffers(1, &element_buffer_);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_indices),
	       plane_indices, GL_STATIC_DRAW);
  check_GLerror();
  
  prog_ = LoadShaders(VERTEX_SHADER_PATH,
		       FRAGMENT_SHADER_PATH);

  int vdatasize = MAX_FILL * nx * ny * nz;

  vdata_.reserve(vdatasize);
  voxel_data vd;
  for (int i = 0; i < vdatasize; i++)
    vdata_.push_back(vd);
  vdata_allocs_.reserve(vdatasize / VOXEL_ALLOC);
  for (int i = 0; i < vdatasize; i++)
    vdata_allocs_.push_back(NULL_CHUNK);

  // Total number of voxels allowed buffered.
  nx_t_ = (nx_ * CHUNK_DIM) * num_chunks_wide_;
  ny_t_ = ny_;
  nz_t_ = (nz_ * CHUNK_DIM) * num_chunks_wide_;
  int size = nx_t_ * ny_t_ * nz_t_;
  voxels_ = new GLint[size];
  dists_  = new   int[size];
  for (int i = 0; i < size; i++) {
    voxels_[i] = 0;
    dists_[i] = 1000;
  }

  // Create buffer for vdata.
  glGenBuffers(1, &gl_vdata_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_vdata_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, vdatasize * sizeof(struct voxel_data),
	       &vdata_[0], GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

  glGenTextures(1, &gl_voxel_tex_);
  glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32I,
	       nx_t_, ny_t_, nz_t_, 0, GL_RED_INTEGER,
	       GL_INT, 0);
  glBindTexture(GL_TEXTURE_3D, 0);

  check_GLerror();

  populate_chunk_region(0, 0, 0, 0, num_chunks_wide_, num_chunks_wide_);

  std::cout << vdata_.size() << " voxels." << std::endl;
  std::cout << "Each voxel uses " << sizeof(struct voxel_data) << " bytes." << std::endl;
  std::cout << "Max voxels in box: " << nx * ny * nz << std::endl;
  std::cout << "Total mem: " << sizeof(struct voxel_data) * vdata_.size()
    + nx * ny * nz * 4 << " bytes." << std::endl;

  std::vector<glm::vec3> raydirs;

  // See https://www.cmu.edu/biolphys/deserno/pdf/sphere_equi.pdf
  // TODO: I think there are bugs here. Figure it out!
  double a = 4.0 * PI/ NUMRAYS; // BUG: Setting 4.0 -> 1.0 causes a segfault... WAT?
  double d1 = std::sqrt(a);
  double mtheta = round(PI/d1);
  double dtheta = PI/mtheta;
  double dphi = a/dtheta;
  for (int m = 0; m < mtheta; m++) {
    double theta = PI*(.5+m)/mtheta;
    double mphi = round(2*PI*sin(theta)/dphi);
    for (int n = 0; n < mphi; n++) {
      double phi = 2 * PI * n / mphi;
      raydirs.push_back(glm::vec3(sin(theta)*cos(phi),
				  sin(theta)*sin(phi),
				  cos(theta)));
    }
  }
  std::random_shuffle(raydirs.begin(), raydirs.end());

  numrays_ = raydirs.size();

  int idx = 0;
  GLint rays[raydirs.size() * RAYLEN * 3];
  
  for (auto ray : raydirs) {
    std::vector<glm::ivec3> raysteps = makeray(RAYLEN, ray);
    for (auto step : raysteps) {
      rays[idx++] = step.x;
      rays[idx++] = step.y;
      rays[idx++] = step.z;
    }
  }

  glGenTextures(1, &gl_raydata_);
  glBindTexture(GL_TEXTURE_2D, gl_raydata_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32I,
	       RAYLEN, raydirs.size(), 0, GL_RGB_INTEGER,
	       GL_INT, 0);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RAYLEN, raydirs.size(), GL_RGB_INTEGER,
		  GL_INT, rays);

  check_GLerror();

  t = 0;
};

VoxelShader::~VoxelShader()
{
  glDeleteBuffers(1, &vertex_buffer_);
  glDeleteBuffers(1, &element_buffer_);
  
  glDeleteTextures(1, &gl_voxel_tex_);

  glDeleteVertexArrays(1, &vao_);
  
  delete[] voxels_;
}

void VoxelShader::draw(int w, int h, float xoff, float yoff, bool perform_update)
{
  GLuint loc;
  glUseProgram(prog_);

  // visible voxel corner in world
  int xpos = (int)xoff * 3;
  int zpos = (int)yoff * 3;

  // visible voxel corner in buffer
  int voxelx = xpos % nx_t_;
  int voxelz = zpos % nz_t_;
  if (voxelx < 0) voxelx += nx_t_;
  if (voxelz < 0) voxelz += nz_t_;

  // buffered voxel corner in world
  int min_voxelx = xpos - (int)(nx_ * CHUNK_DIM) * REQ_BUFFER;
  int min_voxelz = zpos - (int)(nz_ * CHUNK_DIM) * REQ_BUFFER;

  // buffered chunk corner in world
  int min_chunkx = min_voxelx / (nx_ * CHUNK_DIM);
  int min_chunkz = min_voxelz / (nz_ * CHUNK_DIM);

  // buffered chunk corner in buffer
  int voxel_chunkx = min_chunkx % num_chunks_wide_;
  int voxel_chunkz = min_chunkz % num_chunks_wide_;
  if (voxel_chunkx < 0) voxel_chunkx += num_chunks_wide_;
  if (voxel_chunkz < 0) voxel_chunkz += num_chunks_wide_;

  // dimension of chunks to be overwritten.
  int numchunks = num_chunks_wide_  + 2 * (REQ_BUFFER - BUFFER);

  populate_chunk_region(min_chunkx, min_chunkz, voxel_chunkx, voxel_chunkz,
			numchunks, numchunks);

  // Pass everything into the shader.
  loc = glGetUniformLocation(prog_, "wsize");
  glUniform2i(loc, w, h);
  loc = glGetUniformLocation(prog_, "corner");
  glUniform3f(loc, x_, y_, z_);
  loc = glGetUniformLocation(prog_, "dim");
  glUniform3f(loc, w_, h_, d_);
  
  loc = glGetUniformLocation(prog_, "voxeloffset");
  glUniform3i(loc, voxelx, 0, voxelz);
  loc = glGetUniformLocation(prog_, "nvoxels");
  glUniform3i(loc, nx_, ny_, nz_);
  loc = glGetUniformLocation(prog_, "totalvoxels");
  glUniform3i(loc, nx_t_, ny_t_, nz_t_);
  
  loc = glGetUniformLocation(prog_, "update");
  glUniform1i(loc, perform_update ? 1 : 0);
  loc = glGetUniformLocation(prog_, "raylen");
  glUniform1i(loc, RAYLEN);
  loc = glGetUniformLocation(prog_, "numrays");
  glUniform1i(loc, numrays_);

  loc = glGetUniformLocation(prog_, "time");
  glUniform1i(loc, t++);

  glm::vec3 dir;
  vector<glm::ivec3> ray = makerandomray(nx_ + ny_ + nz_, &dir);
  loc = glGetUniformLocation(prog_, "ray");
  glUniform3iv(loc, ray.size(), (GLint*)&ray[0]);
  loc = glGetUniformLocation(prog_, "raydir");
  glUniform3f(loc, dir.x, dir.y, dir.z);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
  loc = glGetUniformLocation(prog_, "voxels");
  glUniform1i(loc, 0);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, gl_vdata_);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, gl_raydata_);
  loc = glGetUniformLocation(prog_, "rays");
  glUniform1i(loc, 1);

  check_GLerror();
  
  // Actual draw call.
  glEnableVertexAttribArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_buffer_);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, NULL);
  glDisableVertexAttribArray(0);

  check_GLerror();

  glUseProgram(0);
}

int VoxelShader::to_chunk_pos(int x, int z)
{
  return x * num_chunks_wide_ + z;
}

void VoxelShader::populate_chunk_region(int xstart, int zstart, int chunkx,
					int chunkz, int w, int h)
{
  // Check each chunk in the region requested.
  // Evict any mismatches.
  // Allocate any now-needed chunks.

  int updates = 0;
  for (int xoff = 0; xoff < w; xoff++) {
    for (int zoff = 0; zoff < h; zoff++) {
      int x = xstart + xoff;
      int z = zstart + zoff;
      int cx = chunkx + xoff;
      int cz = chunkz + zoff;
      cx = (cx + num_chunks_wide_) % num_chunks_wide_;
      cz = (cz + num_chunks_wide_) % num_chunks_wide_;
      int chunkpos = to_chunk_pos(cx, cz);
      chunk_id id(x, z);

      if (chunk_contents_[chunkpos] != id) {
	if (chunk_contents_[chunkpos] != NULL_CHUNK)
	  delete_vdata(chunk_contents_[chunkpos]);
	populate_chunk(x, z, cx, cz);
	updates++;
      }
      if (updates > 1) // Cap at one update per frame (for now)
	break;
    }
    if (updates > 1)
      break;
  }

  if (updates > 0) {
    std::cout << "Some updates." << std::endl;
    int chunkw = (int)(nx_ * CHUNK_DIM);
    int chunkd = (int)(nz_ * CHUNK_DIM);
    int lowx = (nx_t_ + xstart * chunkw % nx_t_) % nx_t_;
    int lowz = (nz_t_ + zstart * chunkd % nz_t_) % nz_t_;
    int highx = (lowx + chunkw * w - 1) % nx_t_;
    int highz = (lowz + chunkd * h - 1) % nz_t_;
    solvedists(lowz, lowz, highx, highz);
    glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
    glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx_t_, ny_t_, nz_t_, GL_RED_INTEGER,
		    GL_INT, voxels_);
    glBindTexture(GL_TEXTURE_3D, 0);
    check_GLerror();
  }
}

void VoxelShader::populate_chunk(int x, int z, int voxx, int voxz)
{
  chunk_id id(x, z);
  
  if (chunk_contents_[to_chunk_pos(voxx, voxz)] != NULL_CHUNK) {
    std::cerr << "Attempted to write to allocated chunk." << std::endl;
    exit(1);
  } else {
    chunk_contents_[to_chunk_pos(voxx, voxz)] = id;
  }
  
  x *= nx_ * CHUNK_DIM;
  z *= nz_ * CHUNK_DIM;
  voxx *= nx_ * CHUNK_DIM;
  voxz *= nz_ * CHUNK_DIM;

  std::cout << "Mapping chunk at " << x << ", " << z << " to " << voxx << ", " << voxz << std::endl;
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_vdata_);

  Voxel v;
  voxel_data vd;
  float xscale = w_ / nx_;
  float yscale = h_ / ny_;
  float zscale = d_ / nz_;
  int alloc_left = 0;
  int alloc_pos = -1;
  int cur_alloc = -1;
  for (int xpos = 0; xpos < CHUNK_DIM * nx_; xpos++) {
    for (int ypos = 0; ypos < ny_; ypos++) {
      for (int zpos = 0; zpos < CHUNK_DIM * nz_; zpos++) {
	int pos = to_pos(glm::ivec3(xpos + voxx, ypos, zpos + voxz));
	dists_[pos] = 1000;
	if (data_->voxelAt(vec3(xscale * (xpos + x),
				yscale * (ypos + 0),
				zscale * (zpos + z)), &v)) {
	  vd.r = 255 * v.color.r;
	  vd.g = 255 * v.color.g;
	  vd.b = 255 * v.color.b;
	  vd.emittance = v.emit;
	  vd.diffuse = v.diffuse;

	  vd.illum_r = vd.illum_g = vd.illum_b = 0;
	  vd.numrays = vd.neighbors = 0;
	  vd.flags = 0;
	  vd.lock = 0;
	  if (alloc_left == 0) {
	    if (cur_alloc != -1) {
	      std::cout << "Writing " << alloc_pos - cur_alloc << " voxels." << std::endl;
	      glBufferSubData(GL_SHADER_STORAGE_BUFFER,
			      cur_alloc * sizeof(struct voxel_data),
			      (alloc_pos - cur_alloc) * sizeof(struct voxel_data),
			      &vdata_[cur_alloc]);
	    }
	    alloc_pos = cur_alloc = alloc_vdata(id);
	    alloc_left = VOXEL_ALLOC;
	    if (alloc_pos < 0) {
	      std::cerr << "Out of voxel allocations!" << std::endl;
	      exit(1);
	    }
	  }
	  voxels_[pos] = alloc_pos;
	  vdata_[alloc_pos] = vd;
	  voxel_dists_.push(voxel_dist(glm::ivec3((xpos + voxx) % nx_t_, ypos,
						  (zpos + voxz) % ny_t_), 0));
	  dists_[pos] = 0;
	  alloc_pos++;
	  alloc_left--;
	} else {
	  voxels_[pos] = 0;
	}
      }
    }
  }
  if (cur_alloc != -1) {
    std::cout << "Writing " << alloc_pos - cur_alloc << " voxels." << std::endl;
    glBufferSubData(GL_SHADER_STORAGE_BUFFER,
		    cur_alloc * sizeof(struct voxel_data),
		    (alloc_pos - cur_alloc) * sizeof(struct voxel_data),
		    &vdata_[cur_alloc]);
  }
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

int VoxelShader::alloc_vdata(chunk_id id)
{
  for (int i = 0; i < vdata_allocs_.size(); i++) {
    if (vdata_allocs_[i] == NULL_CHUNK) { // found free alloc
      vdata_allocs_[i] = id;
      return i * VOXEL_ALLOC; // return offset
    }
  }

  return -1; // No alloc
}

void VoxelShader::delete_vdata(chunk_id id)
{
  for (int i = 0; i < vdata_allocs_.size(); i++) {
    if (vdata_allocs_[i] == id)
      vdata_allocs_[i] = NULL_CHUNK;
  }
  for (int i = 0; i < chunk_contents_.size(); i++) {
    if (chunk_contents_[i] == id)
      chunk_contents_[i] = NULL_CHUNK;
  }
}

int VoxelShader::to_pos(glm::ivec3 v)
{
  return v.x * ny_t_ * nz_t_ + v.y * nz_t_ + v.z;
}

void VoxelShader::updatedistpos(glm::ivec3 p, int newdist)
{
  // Wrapping world.
  p.x = (p.x + nx_t_) % nx_t_;
  p.y = (p.y + ny_t_) % ny_t_;
  p.z = (p.z + nz_t_) % nz_t_;
  
  int pos = to_pos(p);
  if (dists_[pos] <= newdist)
    return;
  dists_[pos] = newdist;
  voxel_dists_.push(voxel_dist(p, newdist));
}

void VoxelShader::solvedists(int minx, int minz, int maxx, int maxz)
{
  int updated = 0;
  int count = 0;
  while (!voxel_dists_.empty()) {
    voxel_dist p = voxel_dists_.front();
    voxel_dists_.pop();

    updated++;

    if (p.pos.x != maxx)
      updatedistpos(p.pos + glm::ivec3(1,0,0), p.dist + 1);
    if (p.pos.x != minx)
      updatedistpos(p.pos + glm::ivec3(-1,0,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,1,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,-1,0), p.dist + 1);
    if (p.pos.z != maxz)
      updatedistpos(p.pos + glm::ivec3(0,0,1), p.dist + 1);
    if (p.pos.z != minz)
      updatedistpos(p.pos + glm::ivec3(0,0,-1), p.dist + 1);
  }

  for (int xpos = 0; xpos < nx_t_; xpos++) {
    for (int ypos = 0; ypos < ny_t_; ypos++) {
      for (int zpos = 0; zpos < nz_t_; zpos++) {
	int pos = to_pos(glm::ivec3(xpos,ypos,zpos));
	if (voxels_[pos] <= 0)
	  voxels_[pos] = -dists_[pos];
	else
	  count ++;
      }
    }
  }

  std::cout << "Visited " << count << " voxels while finding new dists for " <<
    updated << " spaces." << std::endl;
}

std::vector<glm::ivec3> VoxelShader::makeray(int maxlen, glm::vec3 dir)
{
  // Find steps to sample path, starting in center of voxel.
  glm::vec3 maxs(abs(dir.x * .5),
		 abs(dir.y * .5),
		 abs(dir.z * .5));
  glm::vec3 deltas(abs(1.0 / dir.x),
		   abs(1.0 / dir.y),
		   abs(1.0 / dir.z));
  glm::ivec3 step(sign(dir.x), sign(dir.y), sign(dir.z));
  glm::ivec3 pos(0,0,0);
  std::vector<glm::ivec3> steps;

  for (int i = 0; i < maxlen; i++) {
    steps.push_back(pos);
    if (maxs.x < maxs.y && maxs.x < maxs.z) {
      pos.x += step.x;
      maxs.x += deltas.x;
    } else if (maxs.y < maxs.z) {
      pos.y += step.y;
      maxs.y += deltas.y;
    } else {
      pos.z += step.z;
      maxs.z += deltas.z;
    }
  }

  return steps;
}

std::vector<glm::ivec3> VoxelShader::makerandomray(int maxlen, glm::vec3 *dir_out)
{
  // Sample direction on an eigth sphere.
  double r1 = 1.0 - 2.0 * ((float)rand() / RAND_MAX);
  double r2 = (float)rand() / RAND_MAX;
  double r = glm::sqrt(1.0 - r1 * r1);
  double phi = 2 * 3.14159 * r2;
  glm::vec3 dir(abs(r1), abs(cos(phi) * r), abs(sin(phi) * r));
  *dir_out = dir;

  return makeray(maxlen, dir);
}

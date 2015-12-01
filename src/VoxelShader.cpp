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

  vdata_.reserve(MAX_FILL * nx * ny * nz);
  voxel_data vd;
  for (int i = 0; i < vdata_.capacity(); i++)
    vdata_.push_back(vd);
  vdata_allocs_.reserve(vdata_.size() / VOXEL_ALLOC);
  for (int i = 0; i < vdata_.size(); i++)
    vdata_allocs_.push_back(NULL_CHUNK);
  int size = nx * ny * nz;
  voxels_ = new GLint[size];
  dists_  = new   int[size];

  for (int x = 0; x < 4; x++)
    for (int y = 0; y < 4; y++)
      populate_chunk(x,y,x,y);

  solvedists();

  for (int xpos = 0; xpos < nx; xpos++) {
    for (int ypos = 0; ypos < ny; ypos++) {
      for (int zpos = 0; zpos < nz; zpos++) {
	int pos = to_pos(glm::ivec3(xpos,ypos,zpos));
	if (voxels_[pos] <= 0)
	  voxels_[pos] = -dists_[pos];
      }
    }
  }

  std::cout << vdata_.size() - 1 << " voxels." << std::endl;
  std::cout << "Each voxel uses " << sizeof(struct voxel_data) << " bytes." << std::endl;
  std::cout << "Max voxels in box: " << nx * ny * nz << std::endl;
  std::cout << "Total mem: " << sizeof(struct voxel_data) * vdata_.size()
    + nx * ny * nz * 4 << " bytes." << std::endl;

  std::vector<glm::vec3> raydirs;

  // See https://www.cmu.edu/biolphys/deserno/pdf/sphere_equi.pdf
  double a = 4.0 * PI/ NUMRAYS;
  double d1 = std::sqrt(a);
  double mtheta = round(PI/d1);
  double dtheta = PI/mtheta;
  double dphi = a/dtheta;
  for (int m = 1; m < mtheta; m++) {
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

  glGenTextures(1, &gl_voxel_tex_);
  glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32I,
	       nx, ny, nz, 0, GL_RED_INTEGER,
	       GL_INT, 0);
  glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, GL_RED_INTEGER,
		  GL_INT, voxels_);

  glGenTextures(1, &gl_raydata_);
  glBindTexture(GL_TEXTURE_2D, gl_raydata_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32I,
	       RAYLEN, raydirs.size(), 0, GL_RGB_INTEGER,
	       GL_INT, 0);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, RAYLEN, raydirs.size(), GL_RGB_INTEGER,
		  GL_INT, rays);

  glGenBuffers(1, &gl_vdata_);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, gl_vdata_);
  glBufferData(GL_SHADER_STORAGE_BUFFER, vdata_.size() * sizeof(struct voxel_data),
	       &vdata_[0], GL_DYNAMIC_COPY);
  glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

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
  
  // Pass everything into the shader.
  loc = glGetUniformLocation(prog_, "wsize");
  glUniform2i(loc, w, h);
  loc = glGetUniformLocation(prog_, "corner");
  glUniform3f(loc, x_, y_, z_);
  loc = glGetUniformLocation(prog_, "dim");
  glUniform3f(loc, w_, h_, d_);
  loc = glGetUniformLocation(prog_, "nvoxels");
  glUniform3i(loc, nx_, ny_, nz_);
  loc = glGetUniformLocation(prog_, "viewoff");
  glUniform2f(loc, xoff, yoff);
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

void VoxelShader::populate_chunk(int x, int z, int voxx, int voxz)
{
  x *= nx_ * CHUNK_DIM;
  z *= nz_ * CHUNK_DIM;
  voxx *= nx_ * CHUNK_DIM;
  voxz *= nz_ * CHUNK_DIM;
  
  chunk_id id(x, z);
  std::cout << "Allocating chunk at " << x << ", " << z << std::endl;

  Voxel v;
  voxel_data vd;
  float xscale = w_ / nx_;
  float yscale = h_ / ny_;
  float zscale = d_ / nz_;
  int alloc_left = 0;
  int alloc_pos = -1;
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
	    // TODO: glBufferSubData that shizstuff.
	    alloc_pos = alloc_vdata(id);
	    alloc_left = VOXEL_ALLOC;
	    if (alloc_pos < 0) {
	      std::cerr << "Out of voxel allocations!" << std::endl;
	      exit(0);
	    }
	    std::cout << "Successful voxel allocation." << std::endl;
	  }
	  voxels_[pos] = alloc_pos;
	  vdata_[alloc_pos] = vd;
	  voxel_dists_.push(voxel_dist(glm::ivec3(xpos + x, ypos, zpos + z), 0));
	  dists_[pos] = 0;
	  alloc_pos++;
	  alloc_left--;
	} else {
	  voxels_[pos] = 0;
	}
      }
    }
  }
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
}

int VoxelShader::to_pos(glm::ivec3 v)
{
  return v.x * ny_ * nz_ + v.y * nz_ + v.z;
}

void VoxelShader::updatedistpos(glm::ivec3 p, int newdist)
{
  // Wrapping world.
  p.x = (p.x + nx_) % nx_;
  p.y = (p.y + ny_) % ny_;
  p.z = (p.z + nz_) % nz_;
  
  int pos = to_pos(p);
  if (dists_[pos] <= newdist)
    return;
  dists_[pos] = newdist;
  voxel_dists_.push(voxel_dist(p, newdist));
}

void VoxelShader::solvedists()
{
  int count = 0;
  while (!voxel_dists_.empty()) {
    count ++;
    voxel_dist p = voxel_dists_.front();
    voxel_dists_.pop();

    updatedistpos(p.pos + glm::ivec3(1,0,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(-1,0,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,1,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,-1,0), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,0,1), p.dist + 1);
    updatedistpos(p.pos + glm::ivec3(0,0,-1), p.dist + 1);
  }
  std::cout << "Visited " << count << " voxels while finding dists." << std::endl;
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

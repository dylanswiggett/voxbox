#include <iostream>
#include <cstdint>

#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

#include "VoxelShader.hpp"
#include "VoxelData.h"
#include "shader_loader.h"

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

  int size = nx * ny * nz;
  voxels_ = new uint32_t[size];

  glGenTextures(1, &gl_voxel_tex_);
  glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  Voxel v;
  float xscale = w / nx;
  float yscale = h / ny;
  float zscale = d / nz;
  for (int xpos = 0; xpos < nx; xpos++) {
    for (int ypos = 0; ypos < ny; ypos++) {
      for (int zpos = 0; zpos < nz; zpos++) {
	int pos = xpos * ny * nz + ypos * nz + zpos;
	if (data_->voxelAt(vec3(x + xscale * xpos,
				y + yscale * ypos,
				z + zscale * zpos), &v)) {
	  voxels_[pos] = 1; // Big for testing...
	} else {
	  voxels_[pos] = 0;
	}
      }
    }
  }

  glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI,
	       nx, ny, nz, 0, GL_RED_INTEGER,
	       GL_UNSIGNED_INT, 0);
  glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, nx, ny, nz, GL_RED_INTEGER,
		  GL_UNSIGNED_INT, voxels_);

  std::cout << "check..." << std::endl;
  check_GLerror();
  std::cout << " done" << std::endl;

  // TODO: Everything w/ the SSBO
};

VoxelShader::~VoxelShader()
{
  glDeleteBuffers(1, &vertex_buffer_);
  glDeleteBuffers(1, &element_buffer_);
  
  glDeleteTextures(1, &gl_voxel_tex_);

  glDeleteVertexArrays(1, &vao_);
  
  delete[] voxels_;
}

void VoxelShader::draw(int w, int h)
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

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_3D, gl_voxel_tex_);
  loc = glGetUniformLocation(prog_, "voxels");
  glUniform1i(loc, 0);

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

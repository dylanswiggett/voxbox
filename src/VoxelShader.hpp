#include <cstdint>
#include <vector>
#include <GL/glew.h>
#include <GL/gl.h>

#include "VoxelData.h"

#define VERTEX_SHADER_PATH   "shaders/shader.vert"
#define FRAGMENT_SHADER_PATH "shaders/shader.frag"

using namespace std;

/*
A single node inside the voxel tree should contain:

Mat properties: color, emittance.
Light properties: Illumination, diffuse reflectance.
GI properties: number of rays.

color = 24 bits, emittance = 8 bits.
illumination = float, diffuse = 8 bits.
num rays = 8 bits. (approx after 255)
8 bits = neighbors populated.

88 bits per voxel.

Seperate 3D tex for raymarching.
32 bit index, 0 = no voxel.
 */

struct voxel_data {
  int r : 8;
  int g : 8;
  int b : 8;
  int emittance : 8;

  int illum_r : 8;
  int illum_g : 8;
  int illum_b : 8;
  
  int diffuse : 8;
  int numrays : 8;
  int neighbors : 8;
  
  int flags : 16;
};

class VoxelShader
{
private:
  VoxelData* data_;
  float x_, y_, z_;
  float w_, h_, d_;
  int nx_, ny_, nz_;
  GLuint prog_;
  GLuint vao_;
  GLuint gl_voxel_tex_, gl_vdata_;
  GLuint vertex_buffer_, element_buffer_;

  GLuint *voxels_;
  vector<struct voxel_data> vdata_;
public:
  VoxelShader(VoxelData* data,
	      float x, float y, float z, // Corner of rendered region.
	      float w, float h, float d, // Size of rendered region.
	      int nx, int ny, int nz); // Number of voxels in rendered region.
  virtual ~VoxelShader();

  void draw(int w, int h);
};

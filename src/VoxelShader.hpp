#include <cstdint>
#include <vector>
#include <queue>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

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
  uint r : 8;
  uint g : 8;
  uint b : 8;
  uint emittance : 8;

  uint illum_r : 16;
  uint illum_g : 16;
  uint illum_b : 16;
  uint numrays : 16;

  uint diffuse : 8;
  uint neighbors : 8; // Unused
  uint flags : 16;

  uint lock : 32;
};

struct voxel_dist {
  glm::ivec3 pos;
  int dist;
  voxel_dist(const glm::ivec3 p, int d) : pos(p), dist(d) {};
  bool operator<(const voxel_dist &other) const {
    if (dist != other.dist) return dist > other.dist; // sort in increasing order.
    if (pos.x != other.pos.x) return pos.x < other.pos.x;
    if (pos.y != other.pos.y) return pos.y < other.pos.y;
    return pos.z < other.pos.z;
  };
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
  GLuint gl_voxel_tex_, gl_vdata_, gl_raydata_;
  GLuint vertex_buffer_, element_buffer_;

  GLint *voxels_;
  vector<struct voxel_data> vdata_;

  int *dists_;
  priority_queue<voxel_dist> voxel_dists_;

  int numrays_;
  
  int t;
public:
  VoxelShader(VoxelData* data,
	      float x, float y, float z, // Corner of rendered region.
	      float w, float h, float d, // Size of rendered region.
	      int nx, int ny, int nz); // Number of voxels in rendered region.
  virtual ~VoxelShader();

  void draw(int w, int h, float xoff, float yoff, bool perform_update);
private:
  int to_pos(glm::ivec3 p);
  void updatedistpos(glm::ivec3 p, int newdist);
  void solvedists();
  std::vector<glm::ivec3> makeray(int maxlen, glm::vec3 dir);
  std::vector<glm::ivec3> makerandomray(int maxlen, glm::vec3 *dir);
};

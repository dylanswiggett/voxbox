#include <cstdint>
#include <vector>
#include <queue>
#include <GL/glew.h>
#include <GL/gl.h>
#include <glm/glm.hpp>

#include "VoxelData.h"

#define VERTEX_SHADER_PATH   "shaders/shader.vert"
#define FRAGMENT_SHADER_PATH "shaders/shader.frag"

// Parameters for dynamic chunk swapping.
#define CHUNK_DIM .25 // Chunk width/height as a multiplier of height.
#define VOXEL_ALLOC 1000 // Voxels allocated in groups of this size.
#define MAX_FILL 1 // Maximum number of voxels as a multiplier of box size.
#define BUFFER 4 // Number of chunks allocated on each side of the box.
#define REQ_BUFFER 2 // Number of chunks required on each side of the box.
#define MAX_CONCUR_BUFFER 2 // Number of chunks to transfer per frame.

using namespace std;

typedef glm::ivec2 chunk_id;

#define NULL_CHUNK chunk_id(1000000,1000000) // Something stupid.

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
  int x_, y_, z_;
  int nx_, ny_, nz_;
  int nx_t_, ny_t_, nz_t_; // voxels + buffer
  GLuint prog_;
  GLuint vao_;
  GLuint gl_voxel_tex_, gl_vdata_, gl_raydata_;
  GLuint vertex_buffer_, element_buffer_;

  int num_chunks_wide_;
  vector<chunk_id> chunk_contents_;

  GLint *voxels_;
  vector<chunk_id> vdata_allocs_;
  vector<struct voxel_data> vdata_;

  int *dists_;
  queue<voxel_dist> voxel_dists_;

  int numrays_;

  int t;
public:
  VoxelShader(VoxelData* data,
	      int x, int y, int z, // Corner of rendered region.
	      int nx, int ny, int nz); // Number of voxels in rendered region.
  virtual ~VoxelShader();

  void draw(int w, int h, float xoff, float yoff, bool perform_update);
private:
  void populate_chunk_region(int x, int z, int w, int h);
  void populate_chunk(int x, int z);
  int alloc_vdata(chunk_id id);
  void delete_vdata(chunk_id id);

  int to_chunk_pos(int x, int z);
  int to_pos(glm::ivec3 p);
  void updatedistpos(glm::ivec3 p, int newdist);
  void solvedists();
  std::vector<glm::ivec3> makeray(int maxlen, glm::vec3 dir);
  std::vector<glm::ivec3> makerandomray(int maxlen, glm::vec3 *dir);
};

#pragma once

#include "VoxelData.h"

typedef struct VoxelOctreeNode {
  VoxelOctreeNode *children[2][2][2];
  int w, h, d;

  // w x h x d  3D array.
  // Should be populated only when in use (for memory savings).
  Voxel *values;
  
  // Global position w/in voxel dataset.
  double x1, y1, z1;
  double x2, y2, z2;
} VoxelOctreeNode;

/*
 * Converts a voxel dataset into an octree.
 */
class VoxelOctree {
 public:
  // Pass in the geometric bounds of the dataset, and the number of voxels it contains.
  VoxelOctree(VoxelData *source, double w, double h, double d, int v_w, int v_h, int v_d);
  virtual ~VoxelOctree();

  virtual void cleanWith(bool (*function)(VoxelOctreeNode*));
  virtual void generate();
  virtual VoxelOctreeNode *getRoot() const;
 private:
  void populateNode(VoxelOctreeNode *node);

  VoxelOctreeNode *root_;
  VoxelData *source_;
  double w_, h_, d_;
  int v_w_, v_h_, d_h_;
};

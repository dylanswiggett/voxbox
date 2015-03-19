#pragma once

#include "VoxelData.h"

typedef struct VoxelOctreeNode {
  VoxelOctreeNode *children[2][2][2];
  int w, h, d;

  // w x h 2D array.
  // Should be populated only when in use (for memory savings).
  Voxel *values;
  
  // Global position w/in voxel dataset.
  double x1, y1, z1;
  double x2, y2, z2;
} VoxelOctreeNode;

/*
 * Lazily converts a voxel dataset into an octree.
 */
class VoxelOctree {
 public:
  VoxelOctree(VoxelData *source, double w, double h);
  virtual ~VoxelOctree();

  virtual void calculateTo(int depth);
  virtual VoxelOctreeNode *getRoot() const;
  virtual void subdivideFrom(VoxelOctreeNode *node);
  virtual void subdivideWith(bool (*function)(VoxelOctreeNode*));
 private:
  VoxelOctreeNode *root_;
  VoxelData *source_;
  int w_, h_;
};

#ifndef H_BVHTRAVERSAL_FXH_
#define H_BVHTRAVERSAL_FXH_

#include "collision.glsl"

bool  bvh_isLeaf(uint _nid);
void  bvh_getParentSiblingId(uint _nid, out uint _parentId, out uint _siblingId);
void  bvh_getChildId(uint _nid, out uint _left, out uint _right);
void  bvh_getNodeBoxes(uint _nid, out Box _box0, out Box _box1);
void  bvh_collide(uint _nid, Ray r, inout ClosestHit closestHit);
void  tlas_collide(uint _nid, Ray r, inout ClosestHit closestHit);
bool  bvh_collide_fast(uint _nid, Ray r, float tmax);
bool  tlas_collide_fast(uint _nid, Ray r, float tmax);

#include "bvhTraversal_inline.glsl"

#define TLAS_COLLIDE
#include "bvhTraversal_inline.glsl"
#undef TLAS_COLLIDE

#endif
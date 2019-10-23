#ifndef H_COLLISION_FXH_
#define H_COLLISION_FXH_

#include "primitive_cpp.fxh"

// MBVH2 traversal loop
int nodeId = 0;
uint64_t bitstack = 0;
int parentId, siblingId;
// cached node links
for(; ;) 
{
// Inner node loop
while(isInner(nodeId)) 
{
// Load node through texture cache
// node links  -> nid
// node bounds -> n0xy, n1xy, nz
int4 nid  = __ldg((int4*)bvh + nodeId);
float4 n0xy = __ldg((float4*)bvh + nodeId + 1);
float4 n1xy = __ldg((float4*)bvh + nodeId + 2);
float4 nz   = __ldg((float4*)bvh + nodeId + 3); 
parentId  = nid.x;
siblingId = nid.y;
// Box intersection
float near0, far0, near1, far1;
intersectBoxes(ray, n0xy, n1xy, nz,near0, far0, near1, far1);
bool hit0 = (far0 >= near0);
boolhit1 = (far1 >= near1); 
if(!hit0 && !hit1)
break;
bitstack <<= 1;
if(hit0 && hit1) 
{
nodeId = (near1 < near0) ? nid.w : nid.z;
bitstack |= 1;
}
else
{
nodeId = hit0 ? nid.z : nid.w;
}
}
// Leaf node
if(!isInner(nodeId)) 
{
...
}
// Backtrackwhile((bitstack & 1) == 0) 
{
if(bitstack == 0)
return;
nodeId = parentId;
int4 nid = *((int4*)bvh + nodeId);
parentId  = nid.x;
siblingId = nid.y;
bitstack >>= 1;
}
nodeId = siblingId;
bitstack ^= 1;
}

#endif
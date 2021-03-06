#ifndef H_BVHTRAVERSAL_FXH_
#define H_BVHTRAVERSAL_FXH_

#include "collision.glsl"

bool  bvh_isLeaf(uint _nid);
void  bvh_getParentSiblingId(uint _nid, out uint _parentId, out uint _siblingId);
void  bvh_getChildId(uint _nid, out uint _left, out uint _right);
void  bvh_getNodeBoxes(uint _nid, out Box _box0, out Box _box1);
void  bvh_collide(uint _nid, Ray r, inout ClosestHit closestHit);
bool  bvh_collide_fast(uint _nid, Ray r, float tmax);

void traverseBvh(Ray r, uint rootId, inout ClosestHit closestHit)
{
	// MBVH2 traversal loop
	uint nodeId = rootId;
	uint bitstack = 0;
	uint parentId, siblingId;

	while(true) 
	{
		// Inner node loop
		while(!bvh_isLeaf(nodeId))
		{
			uint child0Id, child1Id;
			bvh_getChildId(nodeId, child0Id, child1Id);

			Box box0, box1;
			bvh_getNodeBoxes(nodeId, box0, box1);

			// Box intersection
			float d0 = CollideBox(r, box0, 0, closestHit.t, true);
			float d1 = CollideBox(r, box1, 0, closestHit.t, true);
		
			if(d0 < 0 && d1 < 0)
				break;

			bitstack  = bitstack << 1;

			if(d0 >= 0 && d1 >= 0) 
			{
				nodeId = (d1 < d0) ? child1Id : child0Id;
				bitstack = bitstack | 1;
			}
			else
			{
				nodeId = (d0 >= 0) ? child0Id : child1Id;
			}
		}

		if(bvh_isLeaf(nodeId)) 
		{
			bvh_collide(nodeId, r, closestHit);
		}

		bvh_getParentSiblingId(nodeId, parentId, siblingId);

		// Backtrack
		while((bitstack & 1) == 0) 
		{
			if(bitstack == 0)
				return;

			nodeId = parentId;
			bvh_getParentSiblingId(nodeId, parentId, siblingId);
			bitstack = bitstack >> 1;
		}

		nodeId = siblingId;
		bitstack = bitstack ^ 1;
	}
}

bool traverseBvhFast(Ray r, uint rootId, float tmax)
{
	// MBVH2 traversal loop
	uint nodeId = rootId;
	uint bitstack = 0;
	uint parentId, siblingId;

	while(true) 
	{
		// Inner node loop
		while(!bvh_isLeaf(nodeId))
		{
			uint child0Id, child1Id;
			bvh_getChildId(nodeId, child0Id, child1Id);

			Box box0, box1;
			bvh_getNodeBoxes(nodeId, box0, box1);

			// Box intersection
			float d0 = CollideBox(r, box0, 0, tmax, true);
			float d1 = CollideBox(r, box1, 0, tmax, true);
		
			if(d0 < 0 && d1 < 0)
				break;

			bitstack  = bitstack << 1;

			if(d0 >= 0 && d1 >= 0) 
			{
				nodeId = (d1 < d0) ? child1Id : child0Id;
				bitstack = bitstack | 1;
			}
			else
			{
				nodeId = (d0 >= 0) ? child0Id : child1Id;
			}
		}

		if(bvh_isLeaf(nodeId)) 
		{
			if(bvh_collide_fast(nodeId, r, tmax))
				return true;
		}

		bvh_getParentSiblingId(nodeId, parentId, siblingId);

		// Backtrack
		while((bitstack & 1) == 0) 
		{
			if(bitstack == 0)
				return false;

			nodeId = parentId;
			bvh_getParentSiblingId(nodeId, parentId, siblingId);
			bitstack = bitstack >> 1;
		}

		nodeId = siblingId;
		bitstack = bitstack ^ 1;
	}

	return false;
}

#endif
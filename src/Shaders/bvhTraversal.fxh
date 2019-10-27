#ifndef H_BVHTRAVERSAL_FXH_
#define H_BVHTRAVERSAL_FXH_

#include "collision.fxh"

bool  bvh_isLeaf(uint _nid);
void  bvh_getParentSiblingId(uint _nid, out uint _parentId, out uint _siblingId);
void  bvh_getNodeId(uint _nid, out uint _left, out uint _right, out uint _parentId, out uint _siblingId);
void  bvh_getNodeBoxes(uint _nid, out Box _box0, out Box _box1);
bool  bvh_collide(uint _nid, Ray r);

void traverseBvh(Ray r, uint rootId)
{
	// MBVH2 traversal loop
	uint nodeId = rootId;
	uint bitstack = 0;
	uint parentId, siblingId;
	// cached node links
	while(true) 
	{
		// Inner node loop
		while(!bvh_isLeaf(nodeId))
		{
			uint child0Id, child1Id;
			bvh_getNodeId(nodeId, child0Id, child1Id, parentId, siblingId);

			Box box0, box1;
			bvh_getNodeBoxes(nodeId, box0, box1);

			// Box intersection
			float d0 = CollideBox(r, box0);
			float d1 = CollideBox(r, box1);
		
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
			if(bvh_collide(nodeId, r))
				return;
		}

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

#endif
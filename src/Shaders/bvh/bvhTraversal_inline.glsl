
#ifdef TLAS_COLLIDE
uint traverseTlas(Ray r, uint rootId, inout ClosestHit closestHit)
#else
uint traverseBvh(Ray r, uint rootId, inout ClosestHit closestHit)
#endif
{
	// MBVH2 traversal loop
	uint nodeId = rootId;
	uint bitstack = 0;
	uint parentId, siblingId;
	uint numTraversal = 0;

	while(true) 
	{
		// Inner node loop
		while(!bvh_isLeaf(nodeId))
		{
			numTraversal++;

		#ifdef TLAS_COLLIDE
			numTraversal += tlas_collide(nodeId, r, closestHit);
		#endif

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
	#ifdef TLAS_COLLIDE
			numTraversal += tlas_collide(nodeId, r, closestHit);
	#else
			bvh_collide(nodeId, r, closestHit);
	#endif
		}

		bvh_getParentSiblingId(nodeId, parentId, siblingId);

		// Backtrack
		while((bitstack & 1) == 0) 
		{
			if(bitstack == 0)
				return numTraversal;

			nodeId = parentId;
			bvh_getParentSiblingId(nodeId, parentId, siblingId);
			bitstack = bitstack >> 1;
		}

		nodeId = siblingId;
		bitstack = bitstack ^ 1;
	}

	return numTraversal;
}

#ifdef TLAS_COLLIDE
bool traverseTlasFast(Ray r, uint rootId, float tmax)
#else
bool traverseBvhFast(Ray r, uint rootId, float tmax)
#endif
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
	#ifdef TLAS_COLLIDE
			if(tlas_collide_fast(nodeId, r, tmax))
				return true;
	#else
			if (bvh_collide_fast(nodeId, r, tmax))
				return true;
	#endif
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
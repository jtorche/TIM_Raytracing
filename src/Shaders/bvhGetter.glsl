#ifndef H_BVHGETTER_FXH_
#define H_BVHGETTER_FXH_

#include "primitive_cpp.glsl"

bool bvh_isLeaf(uint _nid)
{
	return (_nid & NID_LEAF_BIT) > 0;
}

void bvh_getParentSiblingId(uint _nid, out uint _parentId, out uint _siblingId)
{
	_parentId = g_BvhNodeData[_nid & NID_MASK].nid.x;
	_siblingId = g_BvhNodeData[_nid & NID_MASK].nid.y;
}

void bvh_getChildId(uint _nid, out uint _left, out uint _right)
{
	uint packed = g_BvhNodeData[_nid].nid.z;
	_left = packed & NID_MASK;
	_right = _left + 1;

	_left = _left   | ((packed & NID_LEFT_LEAF_BIT) > 0 ? NID_LEAF_BIT : 0);
	_right = _right | ((packed & NID_RIGHT_LEAF_BIT) > 0 ? NID_LEAF_BIT : 0);
}

void bvh_getNodeBoxes(uint _nid, out Box _box0, out Box _box1)
{
	vec4 n0xy = g_BvhNodeData[_nid].n0xy;
	vec4 n1xy = g_BvhNodeData[_nid].n1xy;
	vec4 nz = g_BvhNodeData[_nid].nz;

	_box0.minExtent.xy = n0xy.xy;
	_box0.maxExtent.xy = n0xy.zw;
	_box1.minExtent.xy = n1xy.xy;
	_box1.maxExtent.xy = n1xy.zw;

	_box0.minExtent.z = nz.x;
	_box0.maxExtent.z = nz.y;
	_box1.minExtent.z = nz.z;
	_box1.maxExtent.z = nz.w;
}

#endif
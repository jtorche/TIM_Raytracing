#ifndef H_BVHGETTER_FXH_
#define H_BVHGETTER_FXH_

#include "primitive_cpp.glsl"

bool bvh_isLeaf(uint _nid)
{
	return (_nid & 0x8000) > 0;
}

void bvh_getParentSiblingId(uint _nid, out uint _parentId, out uint _siblingId)
{
	uint packed = g_BvhNodeData[_nid & 0x7FFF].nid.x;
	_parentId = packed & 0x0000FFFF;
	_siblingId = (packed & 0xFFFF0000) >> 16;
}

void bvh_getChildId(uint _nid, out uint _left, out uint _right)
{
	uint packed = g_BvhNodeData[_nid].nid.y;
	_left = packed & 0x0000FFFF;
	_right = (packed & 0xFFFF0000) >> 16;
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
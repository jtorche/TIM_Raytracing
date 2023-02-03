#ifndef H_STORESH_FXH_
#define H_STORESH_FXH_

#include "lightprob.glsl"

void storeSH9(ivec3 _coord, SH9Color _sh)
{
	imageStore(g_lpfTextures[0], _coord, vec4(_sh.w[0], 0));

	imageStore(g_lpfTextures[1], _coord, vec4(_sh.w[1].r, _sh.w[2].r, _sh.w[3].r, _sh.w[4].r));
	imageStore(g_lpfTextures[2], _coord, vec4(_sh.w[5].r, _sh.w[6].r, _sh.w[7].r, _sh.w[8].r));

	imageStore(g_lpfTextures[3], _coord, vec4(_sh.w[1].g, _sh.w[2].g, _sh.w[3].g, _sh.w[4].g));
	imageStore(g_lpfTextures[4], _coord, vec4(_sh.w[5].g, _sh.w[6].g, _sh.w[7].g, _sh.w[8].g));

	imageStore(g_lpfTextures[5], _coord, vec4(_sh.w[1].b, _sh.w[2].b, _sh.w[3].b, _sh.w[4].b));
	imageStore(g_lpfTextures[6], _coord, vec4(_sh.w[5].b, _sh.w[6].b, _sh.w[7].b, _sh.w[8].b));
}

#endif
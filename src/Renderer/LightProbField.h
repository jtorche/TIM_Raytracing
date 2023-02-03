#pragma once
#include "rtDevice/public/IRenderer.h"

namespace tim
{
    struct LightProbField
    {
        void allocate(IRenderer* _renderer, uvec3 _fieldSize);
        void free(IRenderer* _renderer);
        void fillBindings(std::vector<ImageBinding>& _bindings, u16 _bindPoint) const;

        uvec3 m_fieldSize;
        u32 m_numProbs;
        ImageHandle lightProbFieldR[2];
        ImageHandle lightProbFieldG[2];
        ImageHandle lightProbFieldB[2];
        ImageHandle lightProbFieldY00;
    };
}
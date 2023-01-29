#pragma once
#include "rtDevice/public/IRenderer.h"
#include "BVHBuilder.h"

struct Material;

namespace tim
{
    class BVHGeometry;
    struct BVHBuildParameters;
    class BVHBuilder;

    class BVHData
    {
    public:
        BVHData(IRenderer* _renderer, const BVHGeometry& _geometry);
        ~BVHData();

        void build(BVHBuilder& _builder, const BVHBuildParameters& _bvhParams, const BVHBuildParameters& _tlasParams, bool _useTlasBlas);

        void fillBvhBindings(std::vector<BufferBinding>& _bindings) const;

    private:
        IRenderer* m_renderer;
        const BVHGeometry& m_geometry;
        BufferHandle m_bvhBuffer;
        uvec2 m_bvhTriangleOffsetRange;
        uvec2 m_bvhPrimitiveOffsetRange;
        uvec2 m_bvhMaterialOffsetRange;
        uvec2 m_bvhLightOffsetRange;
        uvec2 m_bvhNodeOffsetRange;
        uvec2 m_bvhLeafDataOffsetRange;
        uvec2 m_bvhBlasHeaderDataOffsetRange;
    };
}
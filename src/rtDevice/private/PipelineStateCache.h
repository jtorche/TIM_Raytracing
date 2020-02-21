#pragma once
#include "timCore/flat_hash_map.h"
#include "timCore/type.h"
#include "ShaderCompiler/ShaderFlags.h"
#include <VEZ.h>

namespace tim
{
    class ShaderCompiler;

    class PipelineStateCache
    {
    public:
        PipelineStateCache(ShaderCompiler& _shaderCompiler) : m_shaderCompiler{ _shaderCompiler } {}
        ~PipelineStateCache();

        void clear();

        VezPipeline getComputePipeline(const ShaderKey& _key);

    private:
        ShaderCompiler& m_shaderCompiler;
        ska::flat_hash_map<ShaderKey, VezPipeline> m_cache;
        std::vector<VkShaderModule> m_allModules;
        std::vector<VezPipeline> m_dirtyPipeline;
    };
}

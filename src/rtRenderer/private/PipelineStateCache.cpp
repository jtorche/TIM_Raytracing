#include "PipelineStateCache.h"
#include "ShaderCompiler/ShaderCompiler.h"
#include "VezRenderer.h"

namespace tim
{
    VezPipeline PipelineStateCache::getComputePipeline(const ShaderKey& _key)
    {
        auto it = m_cache.find(_key);
        if (it != m_cache.end())
            return it->second;

        auto blob = m_shaderCompiler.getShaderMicrocode(_key.fx, _key.flags);
        
        VezShaderModuleCreateInfo createInfo = {};
        createInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        createInfo.codeSize = blob.size();
        createInfo.pCode = (u32 *)blob.data();
        
        VkShaderModule shaderModule = VK_NULL_HANDLE;
        TIM_VK_VERIFY(vezCreateShaderModule(VezRenderer::get().getVkDevice(), &createInfo, &shaderModule));
        m_allModules.push_back(shaderModule);

        VezPipelineShaderStageCreateInfo psoStageCreateInfo = {};
        psoStageCreateInfo.module = shaderModule;
        psoStageCreateInfo.pEntryPoint = "main";

        VezComputePipelineCreateInfo psoCreateInfo = {};
        psoCreateInfo.pStage = &psoStageCreateInfo;

        VezPipeline pipeline;
        TIM_VK_VERIFY(vezCreateComputePipeline(VezRenderer::get().getVkDevice(), &psoCreateInfo, &pipeline));

        m_cache[_key] = pipeline;
        return pipeline;
    }

    PipelineStateCache::~PipelineStateCache()
    {
        clear();
        for (VezPipeline pipeline : m_dirtyPipeline)
            vezDestroyPipeline(VezRenderer::get().getVkDevice(), pipeline);
    }

    void PipelineStateCache::clear()
    {
        for (auto& [key, pso] : m_cache)
            m_dirtyPipeline.push_back(pso);
        
        for (VkShaderModule module : m_allModules)
            vezDestroyShaderModule(VezRenderer::get().getVkDevice(), module);
        
        m_cache.clear();
        m_allModules.clear();
    }
}
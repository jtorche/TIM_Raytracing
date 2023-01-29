#include "lightProbFieldPass.h"
#include "Scene.h"
#include "TextureManager.h"
#include "BVHData.h"
#include "Shaders/struct_cpp.glsl"
#include "Shaders/lightprob.glsl"

namespace tim
{
	LightProbFieldPass::LightProbFieldPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager) 
		: m_renderer{ _renderer }, m_context{ _context }, m_resourceAllocator{ _allocator }, m_textureManager{ _texManager }
	{
		m_fieldSize = { 20, 20, 10 };
	}

	void LightProbFieldPass::generateLPF(const Scene& _scene)
	{
		const u32 numProbs = m_fieldSize.x * m_fieldSize.y * m_fieldSize.z;
		const u32 irradianceBufferSize = sizeof(vec3) * NUM_RAYS_PER_PROB * numProbs;
		BufferHandle irradianceBuffer = m_resourceAllocator.allocBuffer(irradianceBufferSize, BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);

		DrawArguments arg = {};
		std::vector<ImageBinding> imgBinds;
		m_textureManager.fillImageBindings(imgBinds);

		BufferView lpfConstantsBuffer;
		GenLightProbFieldConstants& lpfConstants = *((GenLightProbFieldConstants *)m_renderer->GetDynamicBuffer(sizeof(GenLightProbFieldConstants), lpfConstantsBuffer));
		lpfConstants.lpfMin = { _scene.getAABB().minExtent, 0 };
		lpfConstants.lpfMax = { _scene.getAABB().maxExtent, 0 };
		lpfConstants.lpfResolution = { m_fieldSize, 0 };
		sampleRays(lpfConstants.rays, NUM_RAYS_PER_PROB);

		std::vector<BufferBinding> bindings = {
			{ { irradianceBuffer, 0, irradianceBufferSize }, { 0, g_outputBuffer_bind } },
			{ lpfConstantsBuffer, { 0, g_CstBuffer_bind } }
		};

		_scene.fillGeometryBufferBindings(bindings);
		_scene.getBVH().fillBvhBindings(bindings);

		arg.m_imageBindings = imgBinds.data();
		arg.m_numImageBindings = (u32)imgBinds.size();
		arg.m_bufferBindings = bindings.data();
		arg.m_numBufferBindings = (u32)bindings.size();

		PushConstants constants = { _scene.getTrianglesCount(), _scene.getBlasInstancesCount(), _scene.getPrimitivesCount(), _scene.getLightsCount(), _scene.getNodesCount() };
		arg.m_constants = &constants;
		arg.m_constantSize = sizeof(constants);
		arg.m_key = { TIM_HASH32(genLightProbField.comp), {} };

		u32 numProbBatch = alignUp<u32>(numProbs, UPDATE_LPF_NUM_PROBS_PER_GROUP) / UPDATE_LPF_NUM_PROBS_PER_GROUP;
		m_context->Dispatch(arg, numProbBatch, NUM_RAYS_PER_PROB);

		m_resourceAllocator.releaseBuffer(irradianceBuffer);
	}

	void LightProbFieldPass::sampleRays(vec4 rays[], u32 _count)
	{
		u32 sqrtCount = 0;
		if (_count == 64)
			sqrtCount = 8;
		else if (_count == 256)
			sqrtCount = 16;

		TIM_ASSERT(sqrtCount* sqrtCount == _count);

		u32 test = 0;
		std::uniform_real_distribution<float> uniform01(0, 1);
		for (u32 i = 0; i < sqrtCount; i++)
		{
			for (u32 j = 0; j < sqrtCount; j++)
			{
				vec2 cellStart = { float(i) / sqrtCount, float(j) / sqrtCount };
				vec2 r = { uniform01(m_rand), uniform01(m_rand) };
				r /= sqrtCount;
				r += cellStart;

				float theta = 2 * TIM_PI * r.x;
				float phi = acosf(1 - 2 * r.y);


				float x = sinf(phi) * cosf(theta);
				float y = sinf(phi) * sinf(theta);
				float z = cosf(phi);

				rays[i * sqrtCount + j] = { x,y,z,0};
				rays[i * sqrtCount + j] = { float(test*3),float(test*3+1),float(test*3+2), 0 };
				test++;
				
			}
		}
	}
}
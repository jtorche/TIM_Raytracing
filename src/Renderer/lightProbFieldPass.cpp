#include "lightProbFieldPass.h"
#include "Scene.h"
#include "TextureManager.h"
#include "BVHData.h"
#include "Shaders/struct_cpp.glsl"
#include "Shaders/lightprob.glsl"

namespace tim
{

	void GpuLightProbField::allocate(IRenderer* _renderer, uvec3 _fieldSize)
	{
		ImageCreateInfo descriptor(ImageFormat::RGBA16F, _fieldSize.x, _fieldSize.y, _fieldSize.z, 1, ImageType::Image3D);

		for (u32 i = 0; i < 2; ++i)
		{
			lightProbFieldR[i] = _renderer->CreateImage(descriptor);
			lightProbFieldG[i] = _renderer->CreateImage(descriptor);
			lightProbFieldB[i] = _renderer->CreateImage(descriptor);
		}
		
		lightProbFieldY00 = _renderer->CreateImage(descriptor);
	}

	void GpuLightProbField::free(IRenderer* _renderer)
	{
		for (u32 i = 0; i < 2; ++i)
		{
			_renderer->DestroyImage(lightProbFieldR[i]);
			_renderer->DestroyImage(lightProbFieldG[i]);
			_renderer->DestroyImage(lightProbFieldB[i]);
		}
		
		_renderer->DestroyImage(lightProbFieldY00);
	}

	LightProbFieldPass::LightProbFieldPass(IRenderer* _renderer, IRenderContext* _context, ResourceAllocator& _allocator, TextureManager& _texManager) 
		: m_renderer{ _renderer }, m_context{ _context }, m_resourceAllocator{ _allocator }, m_textureManager{ _texManager }
	{
		m_fieldSize = { 20, 20, 10 };
		m_field.allocate(m_renderer, m_fieldSize);
	}

	LightProbFieldPass::~LightProbFieldPass()
	{
		m_field.free(m_renderer);
	}

	void LightProbFieldPass::updateLightProbField(const Scene& _scene)
	{
		BufferView lpfConstantsBuffer;
		GenLightProbFieldConstants& lpfConstants = *((GenLightProbFieldConstants*)m_renderer->GetDynamicBuffer(sizeof(GenLightProbFieldConstants), lpfConstantsBuffer));
		lpfConstants.lpfMin = { _scene.getAABB().minExtent, 0 };
		lpfConstants.lpfMax = { _scene.getAABB().maxExtent, 0 };
		lpfConstants.lpfResolution = { m_fieldSize, 0 };
		lpfConstants.sunDir = { _scene.getSunData().sunDir, 0 };
		lpfConstants.sunColor = { _scene.getSunData().sunColor, 0 };

		BufferView shCoefsBuffer;
		SH9* shCoefs = (SH9*)m_renderer->GetDynamicBuffer(sizeof(SH9) * NUM_RAYS_PER_PROB, shCoefsBuffer);
		sampleRays(lpfConstants.rays, shCoefs, NUM_RAYS_PER_PROB);

		BufferView irradianceField = generateIrradiance(lpfConstantsBuffer, _scene);

		DrawArguments arg = {};

		std::vector<BufferBinding> bindings = {
			{ lpfConstantsBuffer, { 0, 0 } },
			{ shCoefsBuffer, { 0, 1 } },
			{ irradianceField, { 0, 2 } }
		};

		std::vector<ImageBinding> imgBinds;
		for (u32 i = 0; i < 2; ++i)
		{
			imgBinds.push_back({ m_field.lightProbFieldR[i], ImageViewType::Storage, { 0, 3, i } });
			imgBinds.push_back({ m_field.lightProbFieldG[i], ImageViewType::Storage, { 0, 4, i } });
			imgBinds.push_back({ m_field.lightProbFieldB[i], ImageViewType::Storage, { 0, 5, i } });
		}
		imgBinds.push_back({ m_field.lightProbFieldY00, ImageViewType::Storage, { 0, 6, 0 } });

		arg.m_imageBindings = imgBinds.data();
		arg.m_numImageBindings = (u32)imgBinds.size();
		arg.m_bufferBindings = bindings.data();
		arg.m_numBufferBindings = (u32)bindings.size();

		arg.m_constants = &m_fieldSize;
		arg.m_constantSize = sizeof(uvec3);
		arg.m_key = { TIM_HASH32(updateLightProbField.comp), {} };

		const u32 numProbs = m_fieldSize.x * m_fieldSize.y * m_fieldSize.z;
		u32 numGroup = alignUp<u32>(numProbs, UPDATE_LPF_LOCALSIZE) / UPDATE_LPF_LOCALSIZE;
		m_context->Dispatch(arg, numGroup, 1);

		m_resourceAllocator.releaseBuffer(irradianceField.m_buffer);
	}

	BufferView LightProbFieldPass::generateIrradiance(const BufferView& _lpfConstants, const Scene& _scene)
	{
		const u32 numProbs = m_fieldSize.x * m_fieldSize.y * m_fieldSize.z;
		const u32 irradianceBufferSize = sizeof(vec3) * NUM_RAYS_PER_PROB * numProbs;
		BufferHandle irradianceBuffer = m_resourceAllocator.allocBuffer(irradianceBufferSize, BufferUsage::Storage | BufferUsage::Transfer, MemoryType::Default);
		BufferView irradianceFieldView = { irradianceBuffer, 0, irradianceBufferSize };

		DrawArguments arg = {};
		std::vector<ImageBinding> imgBinds;
		m_textureManager.fillImageBindings(imgBinds);

		std::vector<BufferBinding> bindings = {
			{ irradianceFieldView, { 0, g_outputBuffer_bind } },
			{ _lpfConstants, { 0, g_CstBuffer_bind } }
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

		return irradianceFieldView;
	}

	void LightProbFieldPass::sampleRays(vec4 rays[], SH9 shCoef[], u32 _count)
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

				vec3 N = { x, y, z };
				const u32 rayIndex = i * sqrtCount + j;
				rays[rayIndex] = { N,0 };
				
				// http://www-graphics.stanford.edu/papers/envmap/envmap.pdf
				SH9& coef = shCoef[rayIndex];

				coef.w[SH_Y00] = 0.282095f;

				coef.w[SH_Y11] = 0.488603f * N.y;
				coef.w[SH_Y10] = 0.488603f * N.z;
				coef.w[SH_Y1_1] = 0.488603f * N.x;

				coef.w[SH_Y21] = 1.092548f * N.x * N.z;
				coef.w[SH_Y2_1] = 1.092548f * N.y * N.z;
				coef.w[SH_Y2_2] = 1.092548f * N.x * N.y;
				
				coef.w[SH_Y20] = 0.315392f * (3.0f * N.z * N.z - 1.0f);
				coef.w[SH_Y22] = 0.546274f * (N.x * N.x - N.y * N.y);
			}
		}
	}
}
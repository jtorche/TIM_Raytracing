#pragma once
#include "LightProbField.h"
#include "resourceAllocator.h"
#include "Shaders/struct_cpp.glsl"

namespace tim
{
	void LightProbField::allocate(IRenderer* _renderer, uvec3 _fieldSize)
	{
		m_fieldSize = _fieldSize;
		m_numProbs = m_fieldSize.x * m_fieldSize.y * m_fieldSize.z;

		ImageCreateInfo descriptor(ImageFormat::RGBA16F, _fieldSize.x, _fieldSize.y, _fieldSize.z, 1, ImageType::Image3D);

		for (u32 i = 0; i < 2; ++i)
		{
			lightProbFieldR[i] = _renderer->CreateImage(descriptor);
			lightProbFieldG[i] = _renderer->CreateImage(descriptor);
			lightProbFieldB[i] = _renderer->CreateImage(descriptor);
		}

		lightProbFieldY00 = _renderer->CreateImage(descriptor);
	}

	void LightProbField::free(IRenderer* _renderer)
	{
		for (u32 i = 0; i < 2; ++i)
		{
			_renderer->DestroyImage(lightProbFieldR[i]);
			_renderer->DestroyImage(lightProbFieldG[i]);
			_renderer->DestroyImage(lightProbFieldB[i]);
		}

		_renderer->DestroyImage(lightProbFieldY00);

		m_fieldSize = {};
		m_numProbs = 0;
	}

	void LightProbField::fillBindings(std::vector<ImageBinding>& _bindings)
	{
		_bindings.push_back({ lightProbFieldY00, ImageViewType::Sampled, { 0, g_lpfTextures_bind, 0 }, SamplerType::Clamp_Linear_MipNearest });

		_bindings.push_back({ lightProbFieldR[0], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 1 }, SamplerType::Clamp_Linear_MipNearest });
		_bindings.push_back({ lightProbFieldR[1], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 2 }, SamplerType::Clamp_Linear_MipNearest });

		_bindings.push_back({ lightProbFieldG[0], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 3 }, SamplerType::Clamp_Linear_MipNearest });
		_bindings.push_back({ lightProbFieldG[1], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 4 }, SamplerType::Clamp_Linear_MipNearest });

		_bindings.push_back({ lightProbFieldB[0], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 5 }, SamplerType::Clamp_Linear_MipNearest });
		_bindings.push_back({ lightProbFieldB[1], ImageViewType::Sampled, { 0, g_lpfTextures_bind, 6 }, SamplerType::Clamp_Linear_MipNearest });
	}
}
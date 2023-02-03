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

		ImageCreateInfo descriptor(ImageFormat::RGBA16F, _fieldSize.x, _fieldSize.y, _fieldSize.z, 1, ImageType::Image3D, MemoryType::Default, ImageUsage::Transfer | ImageUsage::Storage);

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

	void LightProbField::fillBindings(std::vector<ImageBinding>& _bindings, u16 _bindPoint) const
	{
		const SamplerType sampler = SamplerType::Count;
		_bindings.push_back({ lightProbFieldY00, ImageViewType::Storage, { 0, _bindPoint, 0 }, sampler });

		_bindings.push_back({ lightProbFieldR[0], ImageViewType::Storage, { 0, _bindPoint, 1 }, sampler });
		_bindings.push_back({ lightProbFieldR[1], ImageViewType::Storage, { 0, _bindPoint, 2 }, sampler });

		_bindings.push_back({ lightProbFieldG[0], ImageViewType::Storage, { 0, _bindPoint, 3 }, sampler });
		_bindings.push_back({ lightProbFieldG[1], ImageViewType::Storage, { 0, _bindPoint, 4 }, sampler });

		_bindings.push_back({ lightProbFieldB[0], ImageViewType::Storage, { 0, _bindPoint, 5 }, sampler });
		_bindings.push_back({ lightProbFieldB[1], ImageViewType::Storage, { 0, _bindPoint, 6 }, sampler });
	}

	void LightProbField::clearSH(IRenderContext * _context) const
	{
		_context->ClearImage(lightProbFieldY00, Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldR[0], Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldR[1], Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldG[0], Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldG[1], Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldB[0], Color{ 0, 0, 0, 0 });
		_context->ClearImage(lightProbFieldB[1], Color{ 0, 0, 0, 0 });
	}
	
}
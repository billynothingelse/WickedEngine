#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsAPI.h"

namespace Protean::FFTGenerator
{
	void fft_512x512_c2c(
		const Protean::Graphics::GPUResource& pUAV_Dst,
		const Protean::Graphics::GPUResource& pSRV_Dst,
		const Protean::Graphics::GPUResource& pSRV_Src,
		Protean::Graphics::CommandList cmd);

	void LoadShaders();
}

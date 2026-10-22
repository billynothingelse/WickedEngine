#include "rhi/rhi.h"

namespace Protean::Graphics
{
	GraphicsDevice*& GetDevice()
	{
		static GraphicsDevice* device = nullptr;
		return device;
	}
}

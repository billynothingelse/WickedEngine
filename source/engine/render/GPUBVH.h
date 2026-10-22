#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "scene/SceneDecl.h"

namespace Protean
{
	struct GPUBVH
	{
		// Scene BVH intersection resources:
		Protean::Graphics::GPUBuffer bvhNodeBuffer;
		Protean::Graphics::GPUBuffer bvhParentBuffer;
		Protean::Graphics::GPUBuffer bvhFlagBuffer;
		Protean::Graphics::GPUBuffer primitiveCounterBuffer;
		Protean::Graphics::GPUBuffer primitiveIDBuffer;
		Protean::Graphics::GPUBuffer primitiveBuffer;
		Protean::Graphics::GPUBuffer primitiveMortonBuffer;
		uint32_t primitiveCapacity = 0;
		bool IsValid() const { return primitiveCounterBuffer.IsValid(); }

		void Update(const Protean::Scene& scene);
		void Build(const Protean::Scene& scene, Protean::Graphics::CommandList cmd) const;

		void Clear();

		static void Initialize();
	};
}

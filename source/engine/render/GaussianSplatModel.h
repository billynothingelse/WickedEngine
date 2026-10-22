#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "math/Primitive.h"
#include "core/Vector.h"
#include "math/Math.h"
#include "core/ECS.h"
#include "scene/SceneDecl.h"

namespace Protean
{
	class Archive;

	// One gaussian splat asset:
	class GaussianSplatModel
	{
	public:
		Protean::vector<XMFLOAT3> positions;
		Protean::vector<XMFLOAT4> rotations;
		Protean::vector<XMFLOAT3> scales;
		Protean::vector<float> opacities;
		Protean::vector<XMFLOAT3> f_dc;
		Protean::vector<float> f_rest; // number of floats depends on SH degree (L1:9, L2:24, L3:45)

		// Below this are non-serialized attributes:
		Protean::AABB aabb_rest; // aabb without transformation
		Protean::AABB aabb; // aabb with transformation
		XMFLOAT4X4 transform = Protean::Math::IDENTITY_MATRIX;
		XMFLOAT4X4 transform_inverse = Protean::Math::IDENTITY_MATRIX;
		float maxScale = 1;
		Protean::Graphics::GPUBuffer buffer;
		int subresource_splatBuffer = -1;
		int subresource_shBuffer = -1;

		size_t GetSplatCount() const { return positions.size(); };
		int GetSphericalHarmonicsDegree() const;
		size_t GetMemorySizeCPU() const;
		size_t GetMemorySizeGPU() const;

		void CreateRenderData();

		void Update(const XMFLOAT4X4& matrix);

		void Serialize(Protean::Archive& archive, Protean::EntitySerializer& seri);

		static void Initialize();
	};

	// Managing sorting for all gaussian splat models:
	class GaussianSplatScene
	{
	public:
		size_t model_capacity = 0;
		size_t splat_capacity = 0;
		Protean::Graphics::GPUBuffer modelBuffer;
		Protean::Graphics::GPUBuffer indirectBuffer;
		Protean::Graphics::GPUBuffer sortedIndexBuffer;
		Protean::Graphics::GPUBuffer distanceBuffer;
		Protean::Graphics::GPUBuffer splatLookupBuffer;

		constexpr bool IsValid() const { return splat_capacity > 0; }

		void MakeReservations(const GaussianSplatModel* models, size_t model_count);
		void UpdateGPU(const GaussianSplatModel** models, size_t model_count, Protean::Graphics::CommandList cmd, const XMFLOAT4X4* viewmatrices, uint32_t camera_count = 1) const; // culling and sorting. Multiple cameras can be provided when rendering will be performed to multiple destinations
		void Draw(Protean::Graphics::CommandList cmd, uint32_t camera_count = 1) const; // will be drawn with culling and sorting based on previous call to UpdateGPU
		void Clear();
	};
}

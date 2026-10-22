#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsAPI.h"
#include "render/Enums.h"
#include "math/Math.h"
#include "core/ECS.h"
#include "math/Primitive.h"
#include "core/Vector.h"
#include "scene/SceneDecl.h"
#include "scene/SceneComponents.h"
#include "render/shaders/ShaderInterop_HairParticle.h"

namespace Protean
{
	class Archive;
}

namespace Protean
{
	class HairParticleSystem
	{
	public:
		Protean::Graphics::GPUBuffer constantBuffer;
		Protean::Graphics::GPUBuffer generalBuffer;
		Protean::MeshComponent::BufferView simulation_view;
		Protean::MeshComponent::BufferView vb_pos[2];
		Protean::MeshComponent::BufferView vb_nor;
		Protean::MeshComponent::BufferView vb_pos_raytracing;
		Protean::MeshComponent::BufferView vb_uvs;
		Protean::MeshComponent::BufferView wetmap;
		Protean::MeshComponent::BufferView ib_culled;
		Protean::MeshComponent::BufferView indirect_view;
		Protean::MeshComponent::BufferView prim_view;

		Protean::Graphics::GPUBuffer indexBuffer;
		Protean::Graphics::GPUBuffer vertexBuffer_length;

		Protean::Graphics::RaytracingAccelerationStructure BLAS;

		void CreateFromMesh(const Protean::MeshComponent& mesh);
		void DeleteRenderData();
		void CreateRenderData();
		void CreateRaytracingRenderData();

		void UpdateCPU(
			const Protean::TransformComponent& transform,
			const Protean::MeshComponent& mesh,
			float dt
		);

		static void InitializeGPUBuffersIfNeeded(
			const HairParticleSystem* hairs,
			size_t hairCount,
			Protean::Graphics::CommandList cmd
		);

		struct UpdateGPUItem
		{
			const HairParticleSystem* hair = nullptr;
			uint32_t instanceIndex = 0;
			const Protean::MeshComponent* mesh = nullptr;
			const Protean::MaterialComponent* material = nullptr;
		};
		// Update a batch of hair particles by GPU
		static void UpdateGPU(
			const UpdateGPUItem* items,
			uint32_t itemCount,
			Protean::Graphics::CommandList cmd
		);

		void Draw(
			const Protean::MaterialComponent& material,
			Protean::RENDERPASS renderPass,
			Protean::Graphics::CommandList cmd,
			uint16_t camera_mask = 1
		) const;

		Protean::Entity meshID = Protean::INVALID_ENTITY;

		enum FLAGS
		{
			EMPTY = 0,
			_DEPRECATED_REGENERATE_FRAME = 1 << 0,
			REBUILD_BUFFERS = 1 << 1,
			DIRTY = 1 << 2,
			CAMERA_BEND_ENABLED = 1 << 3,
		};
		uint32_t _flags = CAMERA_BEND_ENABLED;

		uint32_t strandCount = 0;
		uint32_t segmentCount = 1;
		uint32_t billboardCount = 1;
		uint32_t randomSeed = 1;
		float length = 1.0f;
		float stiffness = 0.5f;
		float drag = 0.1f;
		float gravityPower = 0;
		float randomness = 0.2f;
		float viewDistance = 200;
		Protean::vector<float> vertex_lengths;
		float width = 1;
		float uniformity = 1;

		struct AtlasRect
		{
			XMFLOAT4 texMulAdd = XMFLOAT4(1, 1, 0, 0);
			float size = 1;
		};
		Protean::vector<AtlasRect> atlas_rects;

		// Non-serialized attributes:
		XMFLOAT4X4 world;
		Protean::AABB aabb;
		Protean::vector<uint32_t> indices; // it is dependent on vertex_lengths and contains triangles with non-zero lengths
		uint32_t layerMask = ~0u;
		mutable bool regenerate_frame = true;
		Protean::Graphics::Format position_format = Protean::Graphics::Format_R16G16B16A16_UNORM;
		mutable bool must_rebuild_blas = true;
		mutable bool gpu_initialized = false;
		// Cached constant buffer last uploaded to the GPU, used to skip
		// redundant per-frame uploads while the contents are unchanged.
		mutable HairParticleCB constantBufferData = {};

		void Serialize(Protean::Archive& archive, Protean::EntitySerializer& seri);

		static void Initialize();

		constexpr uint32_t GetParticleCount() const { return strandCount * segmentCount; }
		constexpr uint32_t GetVertexCount() const { return strandCount * (segmentCount * 2 + 2) * billboardCount; }
		constexpr uint32_t GetIndexCount() const { return 6 * GetParticleCount() * billboardCount; }
		uint64_t GetMemorySizeInBytes() const;

		constexpr bool IsDirty() const { return _flags & DIRTY; }
		constexpr bool IsCameraBendEnabled() const { return _flags & CAMERA_BEND_ENABLED; }

		constexpr void SetDirty(bool value = true) { if (value) { _flags |= DIRTY; } else { _flags &= ~DIRTY; } }
		constexpr void SetCameraBendEnabled(bool value = true) { if (value) { _flags |= CAMERA_BEND_ENABLED; } else { _flags &= ~CAMERA_BEND_ENABLED; } }

		void ConvertFromOLDSpriteSheet(uint32_t framesX, uint32_t framesY, uint32_t frameCount, uint32_t frameStart);
	};
}

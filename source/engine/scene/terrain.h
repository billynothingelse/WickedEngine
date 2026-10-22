#pragma once
#include "core/common_include.h"
#include "scene/scene_decl.h"
#include "scene/scene_components.h"
#include "math/noise.h"
#include "core/ecs.h"
#include "math/color.h"
#include "render/hair_particle.h"
#include "core/vector.h"

#include <memory>

namespace Protean::Terrain
{
	struct Chunk
	{
		union
		{
			struct
			{
				int32_t x, z;
			};
			uint64_t raw = 0;
		};
		constexpr bool operator==(const Chunk& other) const
		{
			return raw == other.raw;
		}
		constexpr uint64_t compute_hash() const
		{
			return raw;
		}
	};
}

namespace std
{
	template <>
	struct hash<Protean::Terrain::Chunk>
	{
		constexpr uint64_t operator()(const Protean::Terrain::Chunk& chunk) const
		{
			return chunk.compute_hash();
		}
	};
}

namespace Protean::Terrain
{
	static constexpr int chunk_width = 64 + 3; // + 3: filler vertices for lod apron and grid perimeter
	static constexpr float chunk_half_width = (chunk_width - 1) * 0.5f;
	static constexpr float chunk_width_rcp = 1.0f / (chunk_width - 1);
	static constexpr uint32_t vertexCount = chunk_width * chunk_width;
	enum
	{
		MATERIAL_BASE,
		MATERIAL_SLOPE,
		MATERIAL_LOW_ALTITUDE,
		MATERIAL_HIGH_ALTITUDE,
		MATERIAL_COUNT
	};

	struct VirtualTextureAtlas
	{
		struct Map
		{
			Protean::Graphics::Texture texture;
			Protean::Graphics::Texture texture_raw_block;
		};
		Map maps[4];
		Protean::Graphics::GPUBuffer tile_pool;

		uint8_t physical_tile_count_x = 0;
		uint8_t physical_tile_count_y = 0;

		struct Tile
		{
			uint8_t x = 0xFF;
			uint8_t y = 0xFF;
			constexpr bool IsValid() const
			{
				return x != 0xFF && y != 0xFF;
			}
			constexpr operator uint16_t() const { return uint16_t(uint16_t(x) | (uint16_t(y) << 8u)); }
		};
		Protean::vector<Tile> free_tiles;

		struct PhysicalTile
		{
			const Tile* last_used = nullptr;
			uint64_t free_frames = 0;
		};
		Protean::vector<PhysicalTile> physical_tiles;

		struct Residency
		{
			Protean::Graphics::Texture feedbackMap;
			Protean::Graphics::Texture residencyMap;
			Protean::Graphics::GPUBuffer requestBuffer;
			Protean::Graphics::GPUBuffer allocationBuffer;
			Protean::Graphics::GPUBuffer allocationBuffer_CPU_readback[Protean::Graphics::GraphicsDevice::GetBufferCount()];
			Protean::Graphics::GPUBuffer pageBuffer;
			Protean::Graphics::GPUBuffer pageBuffer_CPU_upload[Protean::Graphics::GraphicsDevice::GetBufferCount()];
			bool data_available_CPU[Protean::Graphics::GraphicsDevice::GetBufferCount()] = {};
			int16_t cpu_resource_id = 0;
			uint32_t resolution = 0;

			void init(uint32_t resolution);
			void reset();
		};
		Protean::unordered_map<uint32_t, Protean::vector<Protean::Allocator::shared_ptr<Residency>>> free_residencies; // per resolution residencies

		bool allocate_tile(Tile& tile)
		{
			if (free_tiles.empty())
				return false;
			tile = free_tiles.back();
			free_tiles.pop_back();
			PhysicalTile& physical_tile = physical_tiles[tile.x + tile.y * physical_tile_count_x];
			physical_tile.last_used = &tile;
			physical_tile.free_frames = 0;
			return true;
		}
		bool request_residency(const Tile& tile)
		{
			if (check_tile_resident(tile))
			{
				physical_tiles[tile.x + tile.y * physical_tile_count_x].free_frames = 0;
				return true;
			}
			return false;
		}
		bool check_tile_resident(const Tile& tile) const
		{
			if (!tile.IsValid())
				return false;
			return physical_tiles[tile.x + tile.y * physical_tile_count_x].last_used == &tile;
		}
		uint64_t get_tile_frames(const Tile& tile) const
		{
			if (!tile.IsValid())
				return ~0ull;
			return physical_tiles[tile.x + tile.y * physical_tile_count_x].free_frames;
		}
		Protean::Allocator::shared_ptr<Residency> allocate_residency(uint32_t resolution)
		{
			if (free_residencies[resolution].empty())
			{
				Protean::Allocator::shared_ptr<Residency> residency = Protean::Allocator::make_shared<Residency>();
				residency->init(resolution);
				free_residencies[resolution].push_back(residency);
			}
			Protean::Allocator::shared_ptr<Residency> residency = free_residencies[resolution].back();
			free_residencies[resolution].pop_back();
			residency->reset();
			return residency;
		}
		void free_residency(Protean::Allocator::shared_ptr<Residency>& residency)
		{
			if (residency == nullptr)
				return;
			free_residencies[residency->resolution].push_back(residency);
			residency = {};
		}
		inline bool IsValid() const
		{
			return maps[0].texture.IsValid();
		}
	};

	struct VirtualTexture
	{
		Protean::Allocator::shared_ptr<VirtualTextureAtlas::Residency> residency;
		Protean::vector<VirtualTextureAtlas::Tile> tiles;
		uint32_t lod_count = 0;
		uint32_t resolution = 0;

		void init(VirtualTextureAtlas& atlas, uint resolution);

		void free(VirtualTextureAtlas& atlas)
		{
			tiles.clear();
			atlas.free_residency(residency);
		}

		void invalidate()
		{
			resolution = 0;
		}

		struct AllocationRequest
		{
			uint32_t x = 0;
			uint32_t y = 0;
			uint32_t lod = 0;
			uint32_t tile_index = 0;
		};
		Protean::vector<AllocationRequest> allocation_requests;

		// Attach this data to Virtual Texture because we will record these by separate CPU thread:
		struct UpdateRequest
		{
			uint16_t x = 0;
			uint16_t y = 0;
			uint16_t lod = 0;
			uint8_t tile_x = 0;
			uint8_t tile_y = 0;
		};
		mutable Protean::vector<UpdateRequest> update_requests;
		Protean::Graphics::Texture blendmap;
	};

	struct BlendmapLayer
	{
		Protean::vector<uint8_t> pixels;
	};

	struct ChunkData
	{
		Protean::Entity entity = Protean::INVALID_ENTITY;
		Protean::Entity grass_entity = Protean::INVALID_ENTITY;
		Protean::Entity props_entity = Protean::INVALID_ENTITY;
		float prop_density_current = 1;
		float grass_density_current = 1;
		const XMFLOAT3* mesh_vertex_positions = nullptr;
		Protean::HairParticleSystem grass;
		Protean::vector<BlendmapLayer> blendmap_layers;
		Protean::vector<BlendmapLayer> spline_blendmap_layers;
		Protean::Graphics::Texture blendmap;
		Protean::Sphere sphere;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		bool visible = true;
		bool invalidated = false;
		Protean::Allocator::shared_ptr<VirtualTexture> vt;
		Protean::vector<uint16_t> heightmap_data;
		Protean::Graphics::Texture heightmap;

		void enable_blendmap_layer(size_t materialIndex)
		{
			while (blendmap_layers.size() < materialIndex + 1)
			{
				blendmap_layers.emplace_back().pixels.resize(vertexCount);
			}
		}
	};

	struct Prop
	{
		Protean::vector<uint8_t> data; // serialized component data storage
		Protean::Entity source_entity = Protean::INVALID_ENTITY; // the original entity that this prop was serialized from
		int min_count_per_chunk = 0; // a chunk will try to generate min this many props of this type
		int max_count_per_chunk = 10; // a chunk will try to generate max this many props of this type
		int region = 0; // region selection in range [0,3] (0: base/grass, 1: slopes, 2: low altitude (bottom level-0), 3: high altitude (0-top level))
		float region_power = 1; // region weight affection power factor
		float noise_frequency = 1; // perlin noise's frequency for placement factor
		float noise_power = 1; // perlin noise's power
		float threshold = 0.5f; // the chance of placement (higher is less chance)
		float min_size = 1; // scaling randomization range min
		float max_size = 1; // scaling randomization range max
		float min_y_offset = 0; // min randomized offset on Y axis
		float max_y_offset = 0; // max randomized offset on Y axis
	};

	struct Modifier;
	struct Generator;

	struct Terrain
	{
		enum FLAGS
		{
			EMPTY = 0,
			CENTER_TO_CAM = 1 << 0,
			REMOVAL = 1 << 1,
			GRASS = 1 << 2,
			GENERATION_STARTED = 1 << 4,
			PHYSICS = 1 << 5,
			TESSELLATION = 1 << 6,
		};
		uint32_t _flags = CENTER_TO_CAM | REMOVAL | GRASS;

		Protean::Entity terrainEntity = Protean::INVALID_ENTITY;
		Protean::Entity chunkGroupEntity = Protean::INVALID_ENTITY;
		Protean::Scene* scene = nullptr;
		Protean::vector<Protean::Entity> materialEntities;
		Protean::vector<Protean::Entity> splineMaterialEntities;
		Protean::Entity grassEntity = Protean::INVALID_ENTITY;
		Protean::WeatherComponent weather;
		Protean::HairParticleSystem grass_properties;
		Protean::MaterialComponent grass_material;
		Protean::unordered_map<Chunk, ChunkData> chunks;
		Chunk center_chunk;
		Protean::Noise::Perlin perlin_noise;
		Protean::vector<Prop> props;
		int grass_chunk_dist = 1;

		// For generating scene on a background thread:
		float generation_time_budget_milliseconds = 8; // after this much time, the generation thread will start to exit. This can help avoid a very long running, resource consuming and slow cancellation generation
		Protean::Allocator::shared_ptr<Generator> generator;

		Protean::vector<VirtualTexture*> virtual_textures_in_use;
		Protean::Graphics::Sampler sampler;
		VirtualTextureAtlas atlas;

		struct NoSparseCopy
		{
			const Protean::Graphics::Texture* texture_src = nullptr;
			const Protean::Graphics::Texture* texture_dst = nullptr;
			Protean::Graphics::Box srcbox = {};
		};
		mutable Protean::vector<NoSparseCopy> nosparse_copies;

		Protean::Graphics::GPUBuffer chunk_buffer;
		int chunk_buffer_range = 3; // how many chunks to upload to GPU in X and Z directions

		constexpr bool IsCenterToCamEnabled() const { return _flags & CENTER_TO_CAM; }
		constexpr bool IsRemovalEnabled() const { return _flags & REMOVAL; }
		constexpr bool IsGrassEnabled() const { return _flags & GRASS; }
		constexpr bool IsGenerationStarted() const { return _flags & GENERATION_STARTED; }
		constexpr bool IsPhysicsEnabled() const { return _flags & PHYSICS; }
		constexpr bool IsTessellationEnabled() const { return _flags & TESSELLATION; }

		constexpr void SetCenterToCamEnabled(bool value) { if (value) { _flags |= CENTER_TO_CAM; } else { _flags &= ~CENTER_TO_CAM; } }
		constexpr void SetRemovalEnabled(bool value) { if (value) { _flags |= REMOVAL; } else { _flags &= ~REMOVAL; } }
		constexpr void SetGrassEnabled(bool value) { if (value) { _flags |= GRASS; } else { _flags &= ~GRASS; } }
		constexpr void SetGenerationStarted(bool value) { if (value) { _flags |= GENERATION_STARTED; } else { _flags &= ~GENERATION_STARTED; } }
		constexpr void SetPhysicsEnabled(bool value) { if (value) { _flags |= PHYSICS; } else { _flags &= ~PHYSICS; } }
		constexpr void SetTessellationEnabled(bool value) { if (value) { _flags |= TESSELLATION; } else { _flags &= ~TESSELLATION; } }

		float lod_bias = 0;
		int generation = 12;
		int prop_generation = 10;
		int physics_generation = 3;
		float prop_density = 1;
		float grass_density = 1;
		float chunk_scale = 1;
		uint32_t seed = 3926;
		float bottomLevel = -60;
		float topLevel = 380;
		float region1 = 1;
		float region2 = 2;
		float region3 = 8;

		Protean::vector<Protean::Allocator::shared_ptr<Modifier>> modifiers;
		Protean::vector<Modifier*> modifiers_to_remove;

		Terrain();
		~Terrain();

		// Restarts the terrain generation from scratch
		//	This will remove previously existing terrain
		void Generation_Restart();
		// This will run the actual generation tasks, call it once per frame
		void Generation_Update(const Protean::CameraComponent& camera);
		// Tells the generation thread that it should be cancelled and blocks until that is confirmed
		void Generation_Cancel();
		// Check if the terrain generation is currently busy (running in background)
		bool IsGenerationBusy() const;
		// Creates the textures for a chunk data
		void CreateChunkRegionTexture(ChunkData& chunk_data);

		void UpdateVirtualTexturesCPU();
		void UpdateVirtualTexturesGPU(Protean::Graphics::CommandList cmd) const;
		void CopyVirtualTexturePageStatusGPU(Protean::Graphics::CommandList cmd) const;
		void AllocateVirtualTextureTileRequestsGPU(Protean::Graphics::CommandList cmd) const;
		void WritebackTileRequestsGPU(Protean::Graphics::CommandList cmd) const;

		ShaderTerrain GetShaderTerrain() const;

		void InvalidateProps();

		void InvalidateChunksAtSpline(const Protean::SplineComponent& spline);

		void Serialize(Protean::Archive& archive, Protean::EntitySerializer& seri);

	private:
		Protean::vector<Protean::MaterialComponent> materials; // temp storage allocation
		float chunk_scale_rcp = 1.0f / chunk_scale;
	};

	struct Modifier
	{
		virtual ~Modifier() = default;

		enum Type {
			Type_Perlin,
			Type_Voronoi,
			Type_Heightmap,
		} type = Type_Perlin;

		enum BlendMode {
			BlendMode_Normal,
			BlendMode_Additive,
			BlendMode_Multiply,
		} blend = BlendMode_Normal;

		float weight = 0.5f;
		float frequency = 0.0008f;

		// helpers for more user friendly setup with scaling in world space:
		constexpr void SetScale(float scale) { frequency = 1.0f / scale; }
		constexpr float GetScale() const { return 1.0f / frequency; }

		virtual void Seed(uint32_t seed) {}
		virtual void Apply(const XMFLOAT2& world_pos, float& height) = 0;
		constexpr void Blend(float& height, float value)
		{
			switch (blend)
			{
			default:
			case BlendMode_Normal:
				height = Protean::Math::Lerp(height, value, weight);
				break;
			case BlendMode_Multiply:
				height *= value * weight;
				break;
			case BlendMode_Additive:
				height += value * weight;
				break;
			}
		}
	};
	struct PerlinModifier : public Modifier
	{
		int octaves = 6;
		uint32_t seed = 0;
		Protean::Noise::Perlin perlin_noise;

		PerlinModifier() { type = Type_Perlin; }
		void Seed(uint32_t seed) override
		{
			this->seed = seed;
			perlin_noise.init(seed);
		}
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			Blend(height, perlin_noise.compute(p.x, p.y, 0, octaves) * 0.5f + 0.5f);
		}
	};
	struct VoronoiModifier : public Modifier
	{
		float fade = 2.59f;
		float shape = 0.7f;
		float falloff = 6;
		float perturbation = 0.1f;
		uint32_t seed = 0;
		Protean::Noise::Perlin perlin_noise;

		VoronoiModifier() { type = Type_Voronoi; }
		void Seed(uint32_t seed) override
		{
			this->seed = seed;
			perlin_noise.init(seed);
		}
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			if (perturbation > 0)
			{
				const float angle = perlin_noise.compute(p.x, p.y, 0, 6) * XM_2PI;
				p.x += std::sin(angle) * perturbation;
				p.y += std::cos(angle) * perturbation;
			}
			Protean::Noise::Voronoi::Result res = Protean::Noise::Voronoi::compute(p.x, p.y, (float)seed);
			float weight = std::pow(1 - saturate((res.distance - shape) * fade), std::max(0.0001f, falloff));
			Blend(height, weight);
		}
	};
	struct HeightmapModifier : public Modifier
	{
		float amount = 0.1f; // multiplier for height values

		Protean::vector<uint8_t> data;
		int width = 0;
		int height = 0;

		HeightmapModifier() { type = Type_Heightmap; SetScale(1.0f); }
		void Apply(const XMFLOAT2& world_pos, float& height) override
		{
			XMFLOAT2 p = world_pos;
			p.x *= frequency;
			p.y *= frequency;
			XMFLOAT2 pixel = XMFLOAT2(p.x + this->width * 0.5f, p.y + this->height * 0.5f);
			if (pixel.x >= 0 && pixel.x < this->width && pixel.y >= 0 && pixel.y < this->height)
			{
				const int idx = int(pixel.x) + int(pixel.y) * this->width;
				float value = 0;
				if (data.size() == this->width * this->height * sizeof(uint8_t))
				{
					value = ((float)data[idx] / 255.0f);
				}
				else if (data.size() == this->width * this->height * sizeof(uint16_t))
				{
					value = ((float)((uint16_t*)data.data())[idx] / 65535.0f);
				}
				Blend(height, value * amount);
			}
		}
	};

}

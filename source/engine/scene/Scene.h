#pragma once
#include "core/CommonInclude.h"
#include "core/JobSystem.h"
#include "core/SpinLock.h"
#include "render/GPUBVH.h"
#include "render/Sprite.h"
#include "render/SpriteFont.h"
#include "math/Math.h"
#include "core/ECS.h"
#include "scene/SceneComponents.h"
#include "render/EmittedParticle.h"
#include "render/HairParticle.h"
#include "scene/Terrain.h"
#include "math/BVH.h"
#include "core/UnorderedSet.h"
#include "scene/VoxelGrid.h"
#include "scene/PathQuery.h"
#include "render/GaussianSplatModel.h"

#include <string>
#include <memory>

namespace Protean
{
	struct Scene
	{
		virtual ~Scene() = default;

		Protean::ComponentLibrary componentLibrary;

		Protean::ComponentManager<NameComponent>& names = componentLibrary.Register<NameComponent>("Protean::Scene::names");
		Protean::ComponentManager<LayerComponent>& layers = componentLibrary.Register<LayerComponent>("Protean::Scene::layers");
		Protean::ComponentManager<TransformComponent>& transforms = componentLibrary.Register<TransformComponent>("Protean::Scene::transforms");
		Protean::ComponentManager<HierarchyComponent>& hierarchy = componentLibrary.Register<HierarchyComponent>("Protean::Scene::hierarchy");
		Protean::ComponentManager<MaterialComponent>& materials = componentLibrary.Register<MaterialComponent>("Protean::Scene::materials", 11); // version = 11
		Protean::ComponentManager<MeshComponent>& meshes = componentLibrary.Register<MeshComponent>("Protean::Scene::meshes", 4); // version = 4
		Protean::ComponentManager<ImpostorComponent>& impostors = componentLibrary.Register<ImpostorComponent>("Protean::Scene::impostors");
		Protean::ComponentManager<ObjectComponent>& objects = componentLibrary.Register<ObjectComponent>("Protean::Scene::objects", 4); // version = 4
		Protean::ComponentManager<RigidBodyPhysicsComponent>& rigidbodies = componentLibrary.Register<RigidBodyPhysicsComponent>("Protean::Scene::rigidbodies", 6); // version = 6
		Protean::ComponentManager<SoftBodyPhysicsComponent>& softbodies = componentLibrary.Register<SoftBodyPhysicsComponent>("Protean::Scene::softbodies", 3); // version = 3
		Protean::ComponentManager<ArmatureComponent>& armatures = componentLibrary.Register<ArmatureComponent>("Protean::Scene::armatures");
		Protean::ComponentManager<LightComponent>& lights = componentLibrary.Register<LightComponent>("Protean::Scene::lights", 5); // version = 5
		Protean::ComponentManager<CameraComponent>& cameras = componentLibrary.Register<CameraComponent>("Protean::Scene::cameras", 3); // version = 3
		Protean::ComponentManager<EnvironmentProbeComponent>& probes = componentLibrary.Register<EnvironmentProbeComponent>("Protean::Scene::probes", 3); // version = 3
		Protean::ComponentManager<ForceFieldComponent>& forces = componentLibrary.Register<ForceFieldComponent>("Protean::Scene::forces", 1); // version = 1
		Protean::ComponentManager<DecalComponent>& decals = componentLibrary.Register<DecalComponent>("Protean::Scene::decals", 1); // version = 1
		Protean::ComponentManager<AnimationComponent>& animations = componentLibrary.Register<AnimationComponent>("Protean::Scene::animations", 2); // version = 2
		Protean::ComponentManager<AnimationDataComponent>& animation_datas = componentLibrary.Register<AnimationDataComponent>("Protean::Scene::animation_datas");
		Protean::ComponentManager<EmittedParticleSystem>& emitters = componentLibrary.Register<EmittedParticleSystem>("Protean::Scene::emitters", 3); // version = 3
		Protean::ComponentManager<HairParticleSystem>& hairs = componentLibrary.Register<HairParticleSystem>("Protean::Scene::hairs", 3); // version = 3
		Protean::ComponentManager<WeatherComponent>& weathers = componentLibrary.Register<WeatherComponent>("Protean::Scene::weathers", 6); // version = 6
		Protean::ComponentManager<SoundComponent>& sounds = componentLibrary.Register<SoundComponent>("Protean::Scene::sounds", 1); // version = 1
		Protean::ComponentManager<VideoComponent>& videos = componentLibrary.Register<VideoComponent>("Protean::Scene::videos", 2); // version = 2
		Protean::ComponentManager<InverseKinematicsComponent>& inverse_kinematics = componentLibrary.Register<InverseKinematicsComponent>("Protean::Scene::inverse_kinematics");
		Protean::ComponentManager<SpringComponent>& springs = componentLibrary.Register<SpringComponent>("Protean::Scene::springs", 1); // version = 1
		Protean::ComponentManager<ColliderComponent>& colliders = componentLibrary.Register<ColliderComponent>("Protean::Scene::colliders", 2); // version = 2
		Protean::ComponentManager<ScriptComponent>& scripts = componentLibrary.Register<ScriptComponent>("Protean::Scene::scripts");
		Protean::ComponentManager<ExpressionComponent>& expressions = componentLibrary.Register<ExpressionComponent>("Protean::Scene::expressions");
		Protean::ComponentManager<HumanoidComponent>& humanoids = componentLibrary.Register<HumanoidComponent>("Protean::Scene::humanoids", 3); // version = 3
		Protean::ComponentManager<Protean::Terrain::Terrain>& terrains = componentLibrary.Register<Protean::Terrain::Terrain>("Protean::Scene::terrains", 6); // version = 6
		Protean::ComponentManager<Protean::Sprite>& sprites = componentLibrary.Register<Protean::Sprite>("Protean::Scene::sprites", 2); // version = 2
		Protean::ComponentManager<Protean::SpriteFont>& fonts = componentLibrary.Register<Protean::SpriteFont>("Protean::Scene::fonts");
		Protean::ComponentManager<Protean::VoxelGrid>& voxel_grids = componentLibrary.Register<Protean::VoxelGrid>("Protean::Scene::voxel_grids");
		Protean::ComponentManager<MetadataComponent>& metadatas = componentLibrary.Register<MetadataComponent>("Protean::Scene::metadatas");
		Protean::ComponentManager<CharacterComponent>& characters = componentLibrary.Register<CharacterComponent>("Protean::Scene::characters");
		Protean::ComponentManager<PhysicsConstraintComponent>& constraints = componentLibrary.Register<PhysicsConstraintComponent>("Protean::Scene::constraints", 6); // version = 6
		Protean::ComponentManager<SplineComponent>& splines = componentLibrary.Register<SplineComponent>("Protean::Scene::splines", 4); // version = 4
		Protean::ComponentManager<Protean::GaussianSplatModel>& gaussian_splats = componentLibrary.Register<Protean::GaussianSplatModel>("Protean::Scene::gaussian_splats");

		// Non-serialized attributes:
		float dt = 0;
		enum FLAGS
		{
			EMPTY = 0,
		};
		uint32_t flags = EMPTY;

		float time = 0;
		CameraComponent camera; // only for LOD and 3D sound update; use GetCamera() or set RenderPath3D's camera to your own
		Protean::Allocator::shared_ptr<void> physics_scene;
		Protean::SpinLock locker;
		Protean::AABB bounds;
		Protean::vector<Protean::AABB> parallel_bounds;
		WeatherComponent weather;
		uint32_t cloudmap_resolution = 256;
		Protean::Graphics::Texture cloudmap;
		Protean::Graphics::GPUBuffer cloudmap_variance;
		mutable int cloudmap_frame = 0;
		Protean::Graphics::RaytracingAccelerationStructure TLAS;
		Protean::Graphics::GPUBuffer TLAS_instancesUpload[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		void* TLAS_instancesMapped = nullptr;
		Protean::GPUBVH BVH; // this is for non-hardware accelerated raytracing
		mutable bool acceleration_structure_update_requested = false;
		void SetAccelerationStructureUpdateRequested(bool value = true) { acceleration_structure_update_requested = value; }
		bool IsAccelerationStructureUpdateRequested() const { return acceleration_structure_update_requested; }
		bool IsLightmapUpdateRequested() const { return lightmap_request_allocator.load() > 0; }
		Protean::Archive optimized_instatiation_data;
		Protean::vector<Protean::Capsule> character_capsules;
		Protean::vector<Protean::Sphere> character_dedicated_shadows;
		Protean::unordered_map<Protean::Entity, Protean::vector<Protean::Entity>> topdown_hierarchy; // managed by BuildTopDownHierarchy() in every Update(), allows parent->children traversal
		Protean::JobSystem::context topdown_hierarchy_workload;
		uint32_t cpu_gpu_mapped_resource_index = 0;
		mutable uint32_t blas_optimize_offset = 0;
		uint32_t sun_index = 0;

		// AABB culling streams:
		Protean::vector<Protean::AABB> aabb_objects;
		Protean::vector<Protean::AABB> aabb_lights;
		Protean::vector<Protean::AABB> aabb_probes;
		Protean::vector<Protean::AABB> aabb_decals;
		Protean::vector<Protean::AABB> aabb_fonts;

		// Separate stream of world matrices:
		Protean::vector<XMFLOAT4X4> matrix_objects;
		Protean::vector<XMFLOAT4X4> matrix_objects_prev;

		// Shader visible scene parameters:
		ShaderScene shaderscene;

		// Instances for bindless visiblity indexing:
		//	contains in order:
		//		1) objects
		//		2) hair particles
		//		3) emitted particles
		//		4) impostors
		Protean::Graphics::GPUBuffer instanceUploadBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		ShaderMeshInstance* instanceArrayMapped = nullptr;
		size_t instanceArraySize = 0;
		Protean::Graphics::GPUBuffer instanceBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];

		// Geometries for bindless visiblity indexing:
		//	contains in order:
		//		1) meshes * mesh.subsetCount
		//		2) hair particles * 1
		//		3) emitted particles * 1
		//		4) impostors * 1
		Protean::Graphics::GPUBuffer geometryUploadBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		ShaderGeometry* geometryArrayMapped = nullptr;
		size_t geometryArraySize = 0;
		Protean::Graphics::GPUBuffer geometryBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		std::atomic<uint32_t> geometryAllocator{ 0 };

		// Materials for bindless visibility indexing:
		Protean::Graphics::GPUBuffer materialUploadBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		ShaderMaterial* materialArrayMapped = nullptr;
		size_t materialArraySize = 0;
		Protean::Graphics::GPUBuffer materialBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		Protean::Graphics::GPUBuffer textureStreamingFeedbackBuffer;
		Protean::Graphics::GPUBuffer textureStreamingFeedbackBuffer_readback[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		const uint32_t* textureStreamingFeedbackMapped = nullptr;

		// Meshlets:
		Protean::Graphics::GPUBuffer meshletBuffer;
		std::atomic<uint32_t> meshletAllocator{ 0 };

		// Skinning GPU data containining all bones, all morph descriptions:
		Protean::Graphics::GPUBuffer skinningUploadBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		void* skinningDataMapped = nullptr;
		size_t skinningDataSize = 0;
		Protean::Graphics::GPUBuffer skinningBuffer[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		std::atomic<uint32_t> skinningAllocator{ 0 };

		Protean::Graphics::GPUQueryHeap queryHeap;
		Protean::Graphics::GPUBuffer queryResultBuffer[arraysize(OcclusionResult::occlusionQueries)];
		void UpdateOcclusionResult(OcclusionResult& occlusion);
		uint32_t queryheap_idx = 0;
		mutable std::atomic<uint32_t> queryAllocator{ 0 };

		// Surfel GI resources:
		struct SurfelGI
		{
			mutable bool cleared = false;
			Protean::Graphics::GPUBuffer surfelBuffer;
			Protean::Graphics::GPUBuffer dataBuffer;
			Protean::Graphics::GPUBuffer varianceBuffer;
			Protean::Graphics::GPUBuffer aliveBuffer[2];
			Protean::Graphics::GPUBuffer deadBuffer;
			Protean::Graphics::GPUBuffer statsBuffer;
			Protean::Graphics::GPUBuffer indirectBuffer;
			Protean::Graphics::GPUBuffer gridBuffer;
			Protean::Graphics::GPUBuffer cellBuffer;
			Protean::Graphics::GPUBuffer rayBuffer;
			// Ray-sort (coherence) buffers: Morton key + payload (ray slot),
			// radix-sorted so the raytrace traces rays from nearby surfels
			// together.
			Protean::Graphics::GPUBuffer raySortKeyBuffer;
			Protean::Graphics::GPUBuffer raySortPayloadBuffer;
			Protean::Graphics::Texture momentsTexture;
		} surfelgi;

		// DDGI resources:
		struct DDGI
		{
			uint frame_index = 0;
			uint3 grid_dimensions = uint3(32, 8, 32); // The scene extents will be subdivided into a grid of this resolution, each grid cell will have one probe
			float3 grid_min = float3(-1, -1, -1);
			float3 grid_max = float3(1, 1, 1);
			float smooth_backface = 0.01f; // smoothness of backface test
			Protean::Graphics::GPUBuffer ray_buffer;
			Protean::Graphics::GPUBuffer variance_buffer;
			Protean::Graphics::GPUBuffer raycount_buffer;
			Protean::Graphics::GPUBuffer rayallocation_buffer;
			Protean::Graphics::GPUBuffer probe_buffer;
			Protean::Graphics::Texture depth_texture;

			void Serialize(Protean::Archive& archive);
		} ddgi;

		// Voxel GI resources:
		struct VXGI
		{
			uint32_t res = 64;
			float rayStepSize = 1;
			float maxDistance = 100.0f;
			struct ClipMap
			{
				float voxelsize = 0.25;
				XMFLOAT3 center = XMFLOAT3(0, 0, 0);
				XMINT3 offsetfromPrevFrame = XMINT3(0, 0, 0);
				XMFLOAT3 extents = XMFLOAT3(0, 0, 0);
			} clipmaps[VXGI_CLIPMAP_COUNT];
			uint32_t clipmap_to_update = 0;

			Protean::Graphics::Texture radiance;
			Protean::Graphics::Texture prev_radiance;
			Protean::Graphics::Texture render_atomic;
			Protean::Graphics::Texture sdf;
			Protean::Graphics::Texture sdf_temp;
			mutable bool pre_clear = true;
		} vxgi;

		Protean::GaussianSplatScene gaussian_scene;

		EnvironmentProbeComponent global_dynamic_probe; // when no envprobes are placed, this will be the fallback

		// Impostor state:
		static constexpr uint32_t maxImpostorCount = 8;
		static constexpr uint32_t impostorTextureDim = 128;
		Protean::Graphics::Texture impostorDepthStencil;
		Protean::Graphics::Texture impostorRenderTarget_Albedo_MSAA;
		Protean::Graphics::Texture impostorRenderTarget_Normal_MSAA;
		Protean::Graphics::Texture impostorRenderTarget_Surface_MSAA;
		Protean::Graphics::Texture impostorRenderTarget_Albedo;
		Protean::Graphics::Texture impostorRenderTarget_Normal;
		Protean::Graphics::Texture impostorRenderTarget_Surface;
		Protean::Graphics::Texture impostorArray;
		Protean::Graphics::GPUBuffer impostorBuffer;
		uint32_t allocated_impostor_capacity = 0;
		MeshComponent::BufferView impostor_ib32;
		MeshComponent::BufferView impostor_ib16;
		MeshComponent::BufferView impostor_vb_pos;
		MeshComponent::BufferView impostor_vb_nor;
		MeshComponent::BufferView impostor_data;
		MeshComponent::BufferView impostor_indirect;
		Protean::Graphics::Format impostor_ib_format = Protean::Graphics::Format::R32_UINT;
		uint32_t impostorInstanceOffset = ~0u;
		uint32_t impostorGeometryOffset = ~0u;
		uint32_t impostorMaterialOffset = ~0u;

		Protean::EmittedParticleSystem rainEmitter;
		MaterialComponent rainMaterial;
		uint32_t rainInstanceOffset = ~0u;
		uint32_t rainGeometryOffset = ~0u;
		uint32_t rainMaterialOffset = ~0u;
		LightComponent rain_blocker_dummy_light;

		std::atomic<uint32_t> lightmap_request_allocator{ 0 };
		Protean::vector<uint32_t> lightmap_requests;
		Protean::vector<TransformComponent> transforms_temp;

		// CPU/GPU Colliders:
		Protean::vector<uint8_t> collider_deinterleaved_data;
		uint32_t collider_count_cpu = 0;
		uint32_t collider_count_gpu = 0;
		Protean::AABB* aabb_colliders_cpu = nullptr;
		Protean::AABB* aabb_colliders_gpu = nullptr;
		ColliderComponent* colliders_cpu = nullptr;
		ColliderComponent* colliders_gpu = nullptr;
		Protean::BVH collider_bvh;
		Protean::BVH collider_bvh_next;
		Protean::JobSystem::context collider_bvh_workload;
		void CountCPUandGPUColliders();

		// Ocean GPU state:
		Protean::Ocean ocean;
		void OceanRegenerate() { ocean.Create(weather.oceanParameters); }

		// Simple water ripple sprites:
		mutable Protean::vector<Protean::Sprite> waterRipples;
		void PutWaterRipple(const XMFLOAT3& pos);
		void PutWaterRipple(const std::string& image, const XMFLOAT3& pos);

		Protean::Graphics::GPUBuffer voxelgrid_gpu_upload[Protean::Graphics::GraphicsDevice::GetBufferCount()];
		Protean::Graphics::GPUBuffer voxelgrid_gpu[Protean::Graphics::GraphicsDevice::GetBufferCount()]; // primary CPU voxelgrid uploaded to GPU

		// Animation processing optimizer:
		struct AnimationQueue
		{
			// The animations within one queue must be processed on the same thread in order
			Protean::vector<AnimationComponent*> animations; // pointers for one frame only!
			Protean::unordered_set<Protean::Entity> entities;
		};
		Protean::vector<AnimationQueue> animation_queues; // different animation queues can be processed in different threads in any order
		size_t animation_queue_count = 0; // to avoid resizing animation queues downwards because the internals for them needs to be reallocated in that case
		Protean::JobSystem::context animation_dependency_scan_workload;
		void ScanAnimationDependencies();

		Protean::vector<SpringComponent*> spring_queues; // these indicate which chains can be updated on separate threads
		Protean::JobSystem::context spring_dependency_scan_workload;
		void ScanSpringDependencies();
		void UpdateSpringsTopDownRecursive(SpringComponent* parent_spring, SpringComponent& spring);

		float wetmap_fadeout_time = 0;
		bool IsWetmapProcessingRequired() const;

		void StartBuildTopDownHierarchy();
		void WaitBuildTopDownHierarchy() const;
		void RefreshHierarchyTopdownFromParent(Protean::Entity entity);

		// Update all components by a given timestep (in seconds):
		//	This is an expensive function, prefer to call it only once per frame!
		virtual void Update(float dt);
		// Remove everything from the scene that it owns:
		virtual void Clear();
		// Merge an other scene into this.
		//	The contents of the other scene will be lost (and moved to this)!
		//  Any references to entities or components from the other scene will now reference them in this scene.
		virtual void Merge(Scene& other);
		// Similar to merge but skipping some things that are safe to skip within the Update loop
		void MergeFastInternal(Scene& other);
		// Create a copy of prefab and merge it into this.
		//	prefab		: source scene to be copied from
		//	attached	: if true, everything from prefab will be attached to a root entity
		//	returns new root entity if attached is set to true, otherwise returns INVALID_ENTITY
		virtual Protean::Entity Instantiate(Scene& prefab, bool attached = false);
		// Finds all entities in the scene that have any components attached
		void FindAllEntities(Protean::unordered_set<Protean::Entity>& entities) const;

		// Removes (deletes) a specific entity from the scene (if it exists):
		//	recursive	: also removes children if true
		//	keep_sorted	: remove all components while keeping sorted order (slow)
		void Entity_Remove(Protean::Entity entity, bool recursive = true, bool keep_sorted = false);
		// Finds the first entity by the name (if it exists, otherwise returns INVALID_ENTITY):
		//	ancestor : you can specify an ancestor entity if you only want to find entities that are descendants of ancestor entity
		Protean::Entity Entity_FindByName(const std::string& name, Protean::Entity ancestor = Protean::INVALID_ENTITY);
		// Duplicates all of an entity's components and creates a new entity with them (recursively keeps hierarchy):
		Protean::Entity Entity_Duplicate(Protean::Entity entity);
		// Check whether entity is a descendant of ancestor
		//	returns true if entity is in the hierarchy tree of ancestor, false otherwise
		bool Entity_IsDescendant(Protean::Entity entity, Protean::Entity ancestor) const;

		enum class EntitySerializeFlags
		{
			NONE = 0,
			RECURSIVE = 1 << 0, // children entities will be also serialized
			KEEP_INTERNAL_ENTITY_REFERENCES = 1 << 1, // entity handles inside components will be kept intact, they won't use remapping of Protean::EntitySerializer
		};
		// Serializes entity and all of its components to archive:
		//	archive		: archive used for serializing data
		//	seri		: serializer state for entity component system
		//	entity		: if archive is in write mode, this is the entity to serialize. If archive is in read mode, it should be INVALID_ENTITY
		//	flags		: specify options as EntitySerializeFlags bits to control internal behaviour
		//
		//	Returns either the new entity that was read, or the original entity that was written
		Protean::Entity Entity_Serialize(
			Protean::Archive& archive,
			Protean::EntitySerializer& seri,
			Protean::Entity entity = Protean::INVALID_ENTITY,
			EntitySerializeFlags flags = EntitySerializeFlags::RECURSIVE
		);

		Protean::Entity Entity_CreateTransform(
			const std::string& name
		);
		Protean::Entity Entity_CreateMaterial(
			const std::string& name
		);
		Protean::Entity Entity_CreateObject(
			const std::string& name
		);
		Protean::Entity Entity_CreateMesh(
			const std::string& name
		);
		Protean::Entity Entity_CreateLight(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0),
			const XMFLOAT3& color = XMFLOAT3(1, 1, 1),
			float intensity = 1,
			float range = 10,
			LightComponent::LightType type = LightComponent::POINT,
			float outerConeAngle = XM_PIDIV4,
			float innerConeAngle = 0
		);
		Protean::Entity Entity_CreateForce(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		Protean::Entity Entity_CreateEnvironmentProbe(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		Protean::Entity Entity_CreateDecal(
			const std::string& name,
			const std::string& textureName,
			const std::string& normalMapName = ""
		);
		Protean::Entity Entity_CreateCamera(
			const std::string& name,
			float width, float height, float nearPlane = 0.01f, float farPlane = 1000.0f, float fov = XM_PIDIV4
		);
		Protean::Entity Entity_CreateEmitter(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		Protean::Entity Entity_CreateHair(
			const std::string& name,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		Protean::Entity Entity_CreateSound(
			const std::string& name,
			const std::string& filename,
			const XMFLOAT3& position = XMFLOAT3(0, 0, 0)
		);
		Protean::Entity Entity_CreateVideo(
			const std::string& name,
			const std::string& filename
		);
		Protean::Entity Entity_CreateCube(
			const std::string& name
		);
		Protean::Entity Entity_CreatePlane(
			const std::string& name
		);
		Protean::Entity Entity_CreateSphere(
			const std::string& name,
			float radius = 1,
			uint32_t latitudeBands = 64,
			uint32_t longitudeBands = 64
		);
		Protean::Entity Entity_CreateMeshFromData(
			const std::string& name,
			size_t index_count,
			const uint32_t* indices,
			size_t vertex_count,
			const XMFLOAT3* positions
		);

		// Attaches an entity to a parent:
		//	child_already_in_local_space	:	child won't be transformed from world space to local space
		void Component_Attach(Protean::Entity entity, Protean::Entity parent, bool child_already_in_local_space = false);
		// Detaches the entity from its parent (if it is attached):
		void Component_Detach(Protean::Entity entity);
		// Detaches all children from an entity (if there are any):
		void Component_DetachChildren(Protean::Entity parent);
		// Bakes the transform's current world matrix back to hierarchy local space (if it is part of a hierarchy)
		void Component_TransformWorldToHierarchy(Protean::Entity entity);

		// Gathers all direct and indirect children of an entity
		void GatherChildren(Protean::Entity parent, Protean::vector<Protean::Entity>& children) const;

		// Iterates over each child of an entity and executes a lambda function for each child
		//	parent		: the parent entity whose children will be iterated
		//	function	: lambda function that takes a child entity as parameter
		//				  If function returns bool, iteration stops when it returns false
		//				  If function returns void, iteration continues for all children
		template<typename Func>
		void ForEachChild(const Protean::Entity parent, Func function) const
		{
			for (size_t i = 0; i < hierarchy.GetCount(); ++i)
			{
				Protean::Entity child = hierarchy.GetEntity(i);

				if (Entity_IsDescendant(child, parent))
				{
					if constexpr (std::is_same_v<decltype(function(child)), bool>)
					{
						// Function returns bool - stop iteration if it returns false
						if (!function(child))
							break;
					}
					else
					{
						// Function returns void - continue iteration
						function(child);
					}
				}
			}
		}

		// Read/write whole scene into an archive
		void Serialize(Protean::Archive& archive);

		void RunAnimationUpdateSystem(Protean::JobSystem::context& ctx);
		void RunTransformUpdateSystem(Protean::JobSystem::context& ctx);
		void RunHierarchyUpdateSystem(Protean::JobSystem::context& ctx);
		void RunExpressionUpdateSystem(Protean::JobSystem::context& ctx);
		void RunProceduralAnimationUpdateSystem(Protean::JobSystem::context& ctx);
		void RunArmatureUpdateSystem(Protean::JobSystem::context& ctx);
		void RunMeshUpdateSystem(Protean::JobSystem::context& ctx);
		void RunMaterialUpdateSystem(Protean::JobSystem::context& ctx);
		void RunImpostorUpdateSystem(Protean::JobSystem::context& ctx);
		void RunObjectUpdateSystem(Protean::JobSystem::context& ctx);
		void RunCameraUpdateSystem(Protean::JobSystem::context& ctx);
		void RunDecalUpdateSystem(Protean::JobSystem::context& ctx);
		void RunProbeUpdateSystem(Protean::JobSystem::context& ctx);
		void RunForceUpdateSystem(Protean::JobSystem::context& ctx);
		void RunLightUpdateSystem(Protean::JobSystem::context& ctx);
		void RunParticleUpdateSystem(Protean::JobSystem::context& ctx);
		void RunWeatherUpdateSystem(Protean::JobSystem::context& ctx);
		void RunSoundUpdateSystem(Protean::JobSystem::context& ctx);
		void RunVideoUpdateSystem(Protean::JobSystem::context& ctx);
		void RunScriptUpdateSystem(Protean::JobSystem::context& ctx);
		void RunSpriteUpdateSystem(Protean::JobSystem::context& ctx);
		void RunFontUpdateSystem(Protean::JobSystem::context& ctx);
		void RunCharacterUpdateSystem(Protean::JobSystem::context& ctx);
		void RunSplineUpdateSystem(Protean::JobSystem::context& ctx);


		struct RayIntersectionResult
		{
			Protean::Entity entity = Protean::INVALID_ENTITY;
			XMFLOAT3 position = XMFLOAT3(0, 0, 0);
			XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
			XMFLOAT4 uv = XMFLOAT4(0, 0, 0, 0);
			XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
			float distance = FLT_MAX;
			int subsetIndex = -1;
			int vertexID0 = -1;
			int vertexID1 = -1;
			int vertexID2 = -1;
			XMFLOAT2 bary = XMFLOAT2(0, 0);
			XMFLOAT4X4 orientation = Protean::Math::IDENTITY_MATRIX;
			HumanoidComponent::HumanoidBone humanoid_bone = HumanoidComponent::HumanoidBone::Count;

			constexpr bool operator==(const RayIntersectionResult& other) const
			{
				return entity == other.entity;
			}
		};
		// Given a ray, finds the closest intersection point against all mesh instances or collliders
		//	ray				:	the incoming ray that will be traced
		//	filterMask		:	filter based on type
		//	layerMask		:	filter based on layer
		//	lod				:	specify min level of detail for meshes
		RayIntersectionResult Intersects(const Protean::Ray& ray, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// Given a ray, finds the first intersection point against all mesh instances or colliders
		//	returns true immediately if intersection was found, false otherwise
		//	ray				:	the incoming ray that will be traced
		//	filterMask		:	filter based on type
		//	layerMask		:	filter based on layer
		//	lod				:	specify min level of detail for meshes
		bool IntersectsFirst(const Protean::Ray& ray, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// Given a ray, finds all intersections against all mesh instances or collliders
		void IntersectsAll(Protean::vector<RayIntersectionResult>& results, const Protean::Ray& ray, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		struct SphereIntersectionResult
		{
			Protean::Entity entity = Protean::INVALID_ENTITY;
			XMFLOAT3 position = XMFLOAT3(0, 0, 0);
			XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
			XMFLOAT3 velocity = XMFLOAT3(0, 0, 0);
			float depth = 0;
			int subsetIndex = -1;
			XMFLOAT4X4 orientation = Protean::Math::IDENTITY_MATRIX;
			HumanoidComponent::HumanoidBone humanoid_bone = HumanoidComponent::HumanoidBone::Count;
		};
		// Closest sphere intersection
		SphereIntersectionResult Intersects(const Protean::Sphere& sphere, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// All sphere intersections
		void IntersectsAll(Protean::vector<SphereIntersectionResult>& results, const Protean::Sphere& sphere, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		using CapsuleIntersectionResult = SphereIntersectionResult;
		// Closest capsule intersection
		CapsuleIntersectionResult Intersects(const Protean::Capsule& capsule, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// All capsule intersections
		void IntersectsAll(Protean::vector<CapsuleIntersectionResult>& results, const Protean::Capsule& capsule, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, uint32_t lod = 0) const;

		// Goes through the hierarchy backwards and computes entity's world space matrix:
		XMMATRIX ComputeEntityMatrixRecursive(Protean::Entity entity) const;

		// Goes through the hierarchy backwards and computes parent's world space matrix:
		XMMATRIX ComputeParentMatrixRecursive(Protean::Entity entity) const;

		// Retargets an animation from a Humanoid to an other Humanoid such that the new animation will play back on the destination humanoid
		//	dst			:	destination humanoid that the animation will be fit onto
		//	src			:	the animation to copy, it should already target humanoid bones
		//	bake_data	:	if true, the retargeted data will be baked into a new animation data.
		//					if false, it will reuse the source animation data without creating a new one and retargeting will be applied at runtime on every Update
		//	src_scene	:	(optional) specify if you want to retarget from an other scene. This scene must be kept alive while you use the resulting animation if the data is not baked!
		//					If you use a separate scene, then you mustn't serialize the scene while there are animations referencing the src_scene!
		//
		//	returns entity ID of the new animation or INVALID_ENTITY if retargeting was not successful
		Protean::Entity RetargetAnimation(Protean::Entity dst, Protean::Entity src, bool bake_data, const Scene* src_scene = nullptr);

		// If you don't know which armature the bone is contained in, this function can be used to find the first such armature and return the bone's rest matrix
		//	If not found, and entity has a transform, it returns transform matrix
		//	Otherwise, returns identity matrix
		XMMATRIX GetRestPose(Protean::Entity entity) const;
		XMMATRIX FindBoneRestPose(Protean::Entity bone) { return GetRestPose(bone); }; // back-compat of GetRestPose

		// All triangles of the object will be injected into the voxel grid
		//	subtract: if false (default), voxels will be added, if true then voxels will be removed
		void VoxelizeObject(size_t objectIndex, Protean::VoxelGrid& grid, bool subtract = false, uint32_t lod = 0);

		// Voxelize all meshes that match the filters into a voxel grid
		void VoxelizeScene(Protean::VoxelGrid& voxelgrid, bool subtract = false, uint32_t filterMask = Protean::FILTER_ALL, uint32_t layerMask = ~0, uint32_t lod = 0);

		// Get the current position on the surface of an object, tracked by the triangle barycentrics
		XMFLOAT3 GetPositionOnSurface(Protean::Entity objectEntity, int vertexID0, int vertexID1, int vertexID2, const XMFLOAT2& bary) const;

		// Resets pose of the specified entity to bind pose
		//	this will search for all armatures that are descendants of the entity and set all bone matrices to the their bind matrix
		void ResetPose(Protean::Entity entity);

		// Returns the approximate position on the ocean surface seen from a position in world space.
		//	If current weather doesn't have ocean enabled, returns the world position itself.
		//	The result position is approximate because it involves reading back from GPU to the CPU, so the result can be delayed compared to the current GPU simulation.
		//	Note that the input position to this function will be taken on the XZ plane and modified by the displacement map's XZ value, and the Y (vertical) position will be taken from the ocean water height and displacement map only.
		XMFLOAT3 GetOceanPosAt(const XMFLOAT3& worldPosition) const;

		// Computes the LOD for an object AABB for a given view projection matrix
		uint32_t ComputeObjectLODForView(const ObjectComponent& object, const Protean::AABB& aabb, const MeshComponent& mesh, const XMMATRIX& ViewProjection) const;

		// If somehow NANs happened in TransformComponents, this will clear them up and rename them with _nanfix postfix to help filtering them
		void FixupNans();

		// Duplicate colliders will be removed from the scene
		void DeleteDuplicateColliders();

	private:
		void UpdateHumanoidFacings();

	};

	// Returns skinned vertex position
	//	N : normal (out, optional)
	XMVECTOR SkinVertex(const MeshComponent& mesh, const Protean::vector<ShaderTransform>& boneData, uint32_t index, XMVECTOR* N = nullptr);
	// Returns skinned vertex position in armature local space
	//	N : normal (out, optional)
	XMVECTOR SkinVertex(const MeshComponent& mesh, const ArmatureComponent& armature, uint32_t index, XMVECTOR* N = nullptr);
	// Returns skinned vertex position of soft body in world space
	//	N : normal (out, optional)
	XMVECTOR SkinVertex(const MeshComponent& mesh, const SoftBodyPhysicsComponent& softbody, uint32_t index, XMVECTOR* N = nullptr);


	// Helper that manages a global scene
	//	(You don't need to use it, but it's an option for simplicity)
	inline Scene& GetScene()
	{
		static Scene scene;
		return scene;
	}

	// Helper that manages a global camera
	//	(You don't need to use it, but it's an option for simplicity)
	inline CameraComponent& GetCamera()
	{
		static CameraComponent camera;
		return camera;
	}

	// Helper function to open a wiscene file and add the contents to the global scene
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	if true, everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	Protean::Entity LoadModel(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Helper function to open a wiscene file and add the contents to the specified scene. This is thread safe as it doesn't modify global scene
	//	scene			:	the scene that will contain the model
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	attached		:	if true, everything will be attached to a base entity
	//
	//	returns INVALID_ENTITY if attached argument was false, else it returns the base entity handle
	Protean::Entity LoadModel(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), bool attached = false);

	// Helper function to open a wiscene file and add the contents to the global scene
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	rootEntity		:	specify entity to attach whole scene to (optional)
	void LoadModel2(const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), Protean::Entity rootEntity = Protean::INVALID_ENTITY);

	// Helper function to open a wiscene file and add the contents to the specified scene. This is thread safe as it doesn't modify global scene
	//	scene			:	the scene that will contain the model
	//	fileName		:	file path
	//	transformMatrix	:	everything will be transformed by this matrix (optional)
	//	rootEntity		:	specify entity to attach whole scene to (optional)
	void LoadModel2(Scene& scene, const std::string& fileName, const XMMATRIX& transformMatrix = XMMatrixIdentity(), Protean::Entity rootEntity = Protean::INVALID_ENTITY);

	// Deprecated, use Scene::Intersects() function instead
	using PickResult = Scene::RayIntersectionResult;
	PickResult Pick(const Protean::Ray& ray, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

	// Deprecated, use Scene::Intersects() function instead
	using SceneIntersectSphereResult = Scene::SphereIntersectionResult;
	SceneIntersectSphereResult SceneIntersectSphere(const Protean::Sphere& sphere, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

	// Deprecated, use Scene::Intersects() function instead
	using SceneIntersectCapsuleResult = Scene::SphereIntersectionResult;
	SceneIntersectCapsuleResult SceneIntersectCapsule(const Protean::Capsule& capsule, uint32_t filterMask = Protean::FILTER_OPAQUE, uint32_t layerMask = ~0, const Scene& scene = GetScene(), uint32_t lod = 0);

}

template<>
struct enable_bitmask_operators<Protean::Scene::EntitySerializeFlags> : std::true_type {};

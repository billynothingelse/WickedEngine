#pragma once
#include "core/CommonInclude.h"
#include "render/GraphicsAPI/GraphicsDevice.h"
#include "audio/Audio.h"
#include "io/Archive.h"
#include "core/JobSystem.h"
#include "core/Vector.h"
#include "render/Video.h"
#include "core/UnorderedSet.h"
#include "core/UnorderedMap.h"
#include "core/Allocator.h"

#include <memory>
#include <string>

namespace Protean
{
	// This can hold an asset
	//	It can be loaded from file or memory using Protean::ResourceManager::Load()
	struct Resource
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		constexpr bool IsValid() const { return internal_state.IsValid(); }

		const Protean::vector<uint8_t>& GetFileData() const;
		const Protean::Graphics::Texture& GetTexture() const;
		uint32_t GetTextureFullMipCount() const;
		const Protean::Audio::Sound& GetSound() const;
		const std::string& GetScript() const;
		size_t GetScriptHash() const;
		const Protean::Video::Video& GetVideo() const;
		int GetTextureSRGBSubresource() const;
		int GetFontStyle() const;

		void SetFileData(const Protean::vector<uint8_t>& data);
		void SetFileData(Protean::vector<uint8_t>&& data);
		// Allows to set a Texture to the resource from outside
		//	srgb_subresource: you can provide a subresource for SRGB view if the texture is going to be used as SRGB with the GetTextureSRGBSubresource() (optional)
		void SetTexture(const Protean::Graphics::Texture& texture, int srgb_subresource = -1);
		void SetTextureVirtual(const Protean::Graphics::GPUBuffer& tile_pool, const Protean::Graphics::Texture& residency, const Protean::Graphics::Texture& feedback); // for virtual texture, bind additional resources to have same lifetime
		void SetSound(const Protean::Audio::Sound& sound);
		void SetScript(const std::string& script);
		void SetVideo(const Protean::Video::Video& script);

		// Resource marked for recreate on ResourceManager::Load()
		void SetOutdated();

		// Let the streaming system know the required resolution of this resource
		void StreamingRequestResolution(uint32_t resolution);
	};

	namespace ResourceManager
	{
		enum Mode {
			Mode_NO_EMBEDDING,		// default behaviour, serialization will not embed resource file datas
			Mode_EMBED_FILE_DATA,	// serialization will embed file datas if possible

			// legacy modes:
			Mode_DISCARD_FILEDATA_AFTER_LOAD = Mode_NO_EMBEDDING,					// default behaviour: file data will be discarded after loaded. This will not allow serialization of embedded resources, but less memory will be used overall
			Mode_ALLOW_RETAIN_FILEDATA = Mode_EMBED_FILE_DATA,					// allows keeping the file datas around. This mode allows serialization of embedded resources
			Mode_ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING = Mode_NO_EMBEDDING	// allows keeping file datas, but they won't be embedded by serializer
		};
		void SetMode(Mode param);
		Mode GetMode();
		Protean::vector<std::string> GetSupportedImageExtensions();
		Protean::vector<std::string> GetSupportedSoundExtensions();
		Protean::vector<std::string> GetSupportedVideoExtensions();
		Protean::vector<std::string> GetSupportedScriptExtensions();
		Protean::vector<std::string> GetSupportedFontStyleExtensions();

		// Order of these must not change as the flags can be serialized!
		enum Flags {
			Flags_NONE = 0,
			Flags_IMPORT_COLORGRADINGLUT = 1 << 0, // image import will convert resource to 3D color grading LUT
			Flags_IMPORT_RETAIN_FILEDATA = 1 << 1, // file data of the resource will be kept in memory, you will be able to use Resource::GetFileData()
			Flags_IMPORT_NORMALMAP = 1 << 2, // image import will try to use optimal normal map encoding
			Flags_IMPORT_BLOCK_COMPRESSED = 1 << 3, // image import will request block compression for uncompressed or transcodable formats
			Flags_IMPORT_DELAY = 1 << 4, // delay importing resource until later, for example when proper flags can be determined.
			Flags_STREAMING = 1 << 5, // use streaming if possible
		};

		// Load a resource
		//	name : file name of resource
		//	flags : specify flags that modify behaviour (optional)
		//	filedata : pointer to file data, if file was loaded manually (optional)
		//	filesize : size of file data, if file was loaded manually (optional)
		//	container_filename : if name is not the name of source file, set the source file name here
		//	container_fileoffset : if using container_filename, you can give the offset for the resource within the file here
		Resource Load(
			const std::string& name,
			Flags flags = Flags_NONE,
			const uint8_t* filedata = nullptr,
			size_t filesize = ~0ull,
			const std::string& container_filename = "",
			size_t container_fileoffset = 0
		);
		// Check if a resource is currently loaded
		bool Contains(const std::string& name);
		// Invalidate all resources
		void Clear();

		// Set the highest allowed texture asset resolution (only for DDS textures that contain mipmaps)
		void SetTextureResolutionLimit(uint32_t resolution);
		uint32_t GetTextureResolutionLimit();

		// Set threshold relative to memory budget for streaming
		//	If memory usage is below threshold, streaming will work regularly
		//	If memory usage is above threshold, streaming will try to reduce usage
		void SetStreamingMemoryThreshold(float value);
		float GetStreamingMemoryThreshold();

		// Update all streaming resources, call it once per frame on the main thread
		//	Launching or finalizing background streaming jobs is attempted here
		void UpdateStreamingResources(float dt);

		// Returns true if any of the loaded resources are outdated compared to their files
		bool CheckResourcesOutdated();

		// Reload all resources that are outdated
		void ReloadOutdatedResources();

		enum ResourceType {
			ResourceType_TEXTURE		= 1 << 0,
			ResourceType_VIDEO		= 1 << 1,
			ResourceType_SOUND		= 1 << 2,
			ResourceType_SCRIPT		= 1 << 3,
			ResourceType_FONT		= 1 << 4,

			ResourceType_ALL = ResourceType_TEXTURE | ResourceType_VIDEO | ResourceType_SOUND | ResourceType_SCRIPT | ResourceType_FONT
		};

		// Returns a collection of all resources that are alive
		//	you can filter by resource types bitmask
		void CollectResources(Protean::unordered_map<std::string, Protean::Resource>& out_resources, ResourceType types = ResourceType_ALL);

		struct ResourceSerializer
		{
			Protean::vector<Resource> resources;
		};

		void Serialize_READ(Protean::Archive& archive, ResourceSerializer& resources);
		void Serialize_WRITE(Protean::Archive& archive, const Protean::unordered_set<std::string>& resource_names);
	}

}

template<>
struct enable_bitmask_operators<Protean::ResourceManager::Flags> : std::true_type {};
template<>
struct enable_bitmask_operators<Protean::ResourceManager::ResourceType> : std::true_type {};

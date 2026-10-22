#pragma once
#include "core/CommonInclude.h"
#include "audio/Audio.h"
#include "render/GraphicsAPI/GraphicsAPI.h"
#include "core/Vector.h"
#include "core/UnorderedSet.h"

#include <memory>
#include <string>

namespace Protean::Video
{
	struct Video
	{
		std::string title;
		std::string album;
		std::string artist;
		std::string year;
		std::string comment;
		std::string genre;
		uint32_t padded_width = 0;
		uint32_t padded_height = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bit_rate = 0;
		Protean::Graphics::VideoProfile profile = Protean::Graphics::VideoProfile_H264;
		Protean::vector<uint8_t> sps_datas;
		Protean::vector<uint8_t> pps_datas;
		Protean::vector<uint8_t> slice_header_datas;
		uint32_t sps_count = 0;
		uint32_t pps_count = 0;
		uint32_t slice_header_count = 0;
		Protean::Graphics::GPUBuffer data_stream;
		float average_frames_per_second = 0;
		float duration_seconds = 0;
		struct FrameInfo
		{
			uint64_t offset = 0;
			uint64_t size = 0;
			float timestamp_seconds = 0;
			float duration_seconds = 0;
			Protean::Graphics::VideoFrameType type = Protean::Graphics::VideoFrameType_Intra;
			uint32_t reference_priority = 0;
			int poc = 0;
			int gop = 0;
			int display_order = 0;
		};
		Protean::vector<FrameInfo> frame_infos;
		uint32_t num_dpb_slots = 0;
		Protean::Audio::Sound audio;
		inline bool IsValid() const { return data_stream.IsValid(); }
	};

	struct VideoInstance
	{
		const Video* video = nullptr;
		Protean::Graphics::VideoDecoder decoder;
		struct DPB
		{
			Protean::Graphics::Texture texture; // raw decoder image array (only can be sampled when device supports coincide mode decoder)
			int subresources_luminance[17] = {};
			int subresources_chrominance[17] = {};
			int poc_status[17] = {};
			int framenum_status[17] = {};
			Protean::Graphics::ResourceState resource_states[17] = {};
			Protean::vector<uint8_t> reference_usage;
			struct Picture
			{
				bool used = false;
				int frame_num = 0;
				int poc = 0;
			};
			Picture pictures[17] = {};
			uint8_t current_slot = 0;
		} dpb;
		struct OutputTexture
		{
			Protean::Graphics::Texture texture; // resolved RGB image
			int subresource_srgb = -1;
			int display_order = -1;

			// Below can be either point to DPB in coincide mode, or separate decoder output in non-coincide mode:
			Protean::Graphics::Texture src;
			int src_subresource_luminance = -1;
			int src_subresource_chrominance = -1;
		};
		Protean::vector<OutputTexture> output_textures_free; // free images that can be reused for display order buffering
		Protean::vector<OutputTexture> output_textures_resolve_request; // request image to be resolved
		Protean::vector<OutputTexture> output_textures_used; // resolved image for future display ordering use
		OutputTexture output; // Currently displayed RGB image with the latest display order
		int target_display_order = 0; // the current display order that should be visible
		int current_decode_frame = 0; // the latest decoded frame index
		float current_time = 0; // tracking the absolute time of the playback in seconds
		Protean::Audio::SoundInstance audio;
		float audio_time_origin = 0;
		float audio_sync_time = 0;
		bool audio_unavailable = false;
		enum Flags {
			Flags_Empty = 0,
			Flags_Playing = 1 << 0,
			Flags_Looped = 1 << 1,
			Flags_Mipmapped = 1 << 2,
			Flags_NeedsResolve = 1 << 3,
			Flags_InitialFirstFrameDecoded = 1 << 4,
			Flags_DecoderReset = 1 << 5,
		};
		Flags flags = Flags_Empty;
		inline bool IsValid() const { return decoder.IsValid(); }

		// Get texture resource of the latest decoded frame that can be displayed:
		Protean::Graphics::Texture GetCurrentFrameTexture() const { return output.texture; }
		// Get SRGB subresource view of the latest decoded frame that can be displayed:
		int GetCurrentFrameTextureSRGBSubresource() const { return output.subresource_srgb; }
	};

	bool CreateVideo(const std::string& filename, Video* video);
	bool CreateVideoMP4(const uint8_t* filedata, size_t filesize, Video* video);
	bool CreateVideoH264RAW(const uint8_t* filedata, size_t filesize, Video* video);
	bool CreateVideoInstance(const Video* video, VideoInstance* instance);

	void UpdateVideo(VideoInstance* instance, float dt);

	bool IsDecodingRequired(const VideoInstance* instance);
	void DecodeVideo(VideoInstance* instance, Protean::Graphics::CommandList cmd);
	void ResolveVideoToRGB(VideoInstance* instance, Protean::Graphics::CommandList cmd);

	// Set video instance state to a timer (approximately), this will take efect the next time it is decoded
	void Seek(VideoInstance* instance, float timerSeconds);
}

template<>
struct enable_bitmask_operators<Protean::Video::VideoInstance::Flags> : std::true_type {};

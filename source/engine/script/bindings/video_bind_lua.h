#pragma once
#include "script/lua.h"
#include "script/luna.h"
#include "audio/audio.h"
#include "resource/resource_manager.h"

namespace Protean::Lua
{
	class Video_BindLua
	{
	public:
		Protean::Resource videoResource;

		inline static constexpr char className[] = "Video";
		static Luna<Video_BindLua>::FunctionType methods[];
		static Luna<Video_BindLua>::PropertyType properties[];

		Video_BindLua(lua_State* L);
		Video_BindLua(const Protean::Video::Video& video)
		{
			videoResource.SetVideo(video);
		}
		Video_BindLua(const Protean::Resource resource)
		{
			videoResource = resource;
		}

		int IsValid(lua_State* L);
		int GetDurationSeconds(lua_State* L);

		static void Bind();
	};

	class VideoInstance_BindLua
	{
	public:
		Protean::Video::VideoInstance videoinstance;

		inline static constexpr char className[] = "VideoInstance";
		static Luna<VideoInstance_BindLua>::FunctionType methods[];
		static Luna<VideoInstance_BindLua>::PropertyType properties[];

		VideoInstance_BindLua(lua_State* L);
		VideoInstance_BindLua(const Protean::Video::VideoInstance& instance)
		{
			videoinstance = instance;
		}
		~VideoInstance_BindLua() {}

		int IsValid(lua_State* L);

		int Play(lua_State* L);
		int Pause(lua_State* L);
		int Stop(lua_State* L);
		int SetLooped(lua_State* L);
		int Seek(lua_State* L);

		int GetCurrentTimer(lua_State* L);
		int IsEnded(lua_State* L);

		int SetVolume(lua_State* L);
		int GetVolume(lua_State* L);

		static void Bind();
	};

}

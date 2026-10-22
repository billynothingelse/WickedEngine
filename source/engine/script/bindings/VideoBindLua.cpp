#include "script/bindings/VideoBindLua.h"
#include "render/Video.h"

namespace Protean::Lua
{

	Luna<Video_BindLua>::FunctionType Video_BindLua::methods[] = {
		lunamethod(Video_BindLua, IsValid),
		lunamethod(Video_BindLua, GetDurationSeconds),
		{ NULL, NULL }
	};
	Luna<Video_BindLua>::PropertyType Video_BindLua::properties[] = {
		{ NULL, NULL }
	};

	Video_BindLua::Video_BindLua(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			videoResource = Protean::ResourceManager::Load(Protean::Lua::SGetString(L, 1));
		}
	}

	int Video_BindLua::IsValid(lua_State* L)
	{
		Protean::Lua::SSetBool(L, videoResource.IsValid() && videoResource.GetVideo().IsValid());
		return 1;
	}

	int Video_BindLua::GetDurationSeconds(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, videoResource.IsValid() ? videoResource.GetVideo().duration_seconds : 0.0f);
		return 1;
	}

	void Video_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<Video_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}



	Luna<VideoInstance_BindLua>::FunctionType VideoInstance_BindLua::methods[] = {
		lunamethod(VideoInstance_BindLua, IsValid),
		lunamethod(VideoInstance_BindLua, Play),
		lunamethod(VideoInstance_BindLua, Pause),
		lunamethod(VideoInstance_BindLua, Stop),
		lunamethod(VideoInstance_BindLua, SetLooped),
		lunamethod(VideoInstance_BindLua, Seek),
		lunamethod(VideoInstance_BindLua, GetCurrentTimer),
		lunamethod(VideoInstance_BindLua, IsEnded),
		lunamethod(VideoInstance_BindLua, SetVolume),
		lunamethod(VideoInstance_BindLua, GetVolume),
		{ NULL, NULL }
	};
	Luna<VideoInstance_BindLua>::PropertyType VideoInstance_BindLua::properties[] = {
		{ NULL, NULL }
	};

	VideoInstance_BindLua::VideoInstance_BindLua(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Video_BindLua* s = Luna<Video_BindLua>::lightcheck(L, 1);
			if (s == nullptr)
			{
				Protean::Lua::SError(L, "VideoInstance(Video Video) : first argument is not a Video!");
				return;
			}
			if (!s->videoResource.IsValid())
			{
				Protean::Lua::SError(L, "VideoInstance(Video Video, opt float begin,end) : Video is not valid!");
				return;
			}
			const Protean::Video::Video& Video = s->videoResource.GetVideo();
			if (!Video.IsValid())
			{
				Protean::Lua::SError(L, "VideoInstance(Video Video, opt float begin,end) : Video is not valid!");
				return;
			}
			Protean::Video::CreateVideoInstance(&Video, &videoinstance);
		}
	}

	int VideoInstance_BindLua::IsValid(lua_State* L)
	{
		Protean::Lua::SSetBool(L, videoinstance.IsValid());
		return 1;
	}

	int VideoInstance_BindLua::Play(lua_State* L)
	{
		videoinstance.flags |= Protean::Video::VideoInstance::Flags::Playing;
		return 0;
	}
	int VideoInstance_BindLua::Pause(lua_State* L)
	{
		videoinstance.flags &= ~Protean::Video::VideoInstance::Flags::Playing;
		return 0;
	}
	int VideoInstance_BindLua::Stop(lua_State* L)
	{
		videoinstance.flags &= ~Protean::Video::VideoInstance::Flags::Playing;
		Protean::Video::Seek(&videoinstance, 0);
		return 0;
	}
	int VideoInstance_BindLua::SetLooped(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);

		bool looped = true;
		if (argc > 0)
		{
			looped = Protean::Lua::SGetBool(L, 1);
		}

		if (looped)
		{
			videoinstance.flags |= Protean::Video::VideoInstance::Flags::Looped;
		}
		else
		{
			videoinstance.flags &= ~Protean::Video::VideoInstance::Flags::Looped;
		}
		return 0;
	}
	int VideoInstance_BindLua::Seek(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Video::Seek(&videoinstance, Protean::Lua::SGetFloat(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "Seek(float timerSeconds) not enough arguments!");
		}
		return 0;
	}
	int VideoInstance_BindLua::GetCurrentTimer(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, videoinstance.current_time);
		return 1;
	}
	int VideoInstance_BindLua::IsEnded(lua_State* L)
	{
		if (videoinstance.video == nullptr)
		{
			Protean::Lua::SSetBool(L, true);
			return 1;
		}
		Protean::Lua::SSetBool(L, videoinstance.current_time >= videoinstance.video->duration_seconds);
		return 1;
	}

	int VideoInstance_BindLua::SetVolume(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Audio::SetVolume(Protean::Lua::SGetFloat(L, 1), &videoinstance.audio);
		}
		else
		{
			Protean::Lua::SError(L, "SetVolume(float volume) not enough arguments!");
		}
		return 0;
	}
	int VideoInstance_BindLua::GetVolume(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, Protean::Audio::GetVolume(&videoinstance.audio));
		return 1;
	}

	void VideoInstance_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<VideoInstance_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}


}

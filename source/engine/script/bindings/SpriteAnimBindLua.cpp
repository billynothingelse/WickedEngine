#include "script/bindings/SpriteAnimBindLua.h"
#include "script/bindings/MathBindLua.h"

namespace Protean::Lua
{

	Luna<SpriteAnim_BindLua>::FunctionType SpriteAnim_BindLua::methods[] = {
		lunamethod(SpriteAnim_BindLua, SetRot),
		lunamethod(SpriteAnim_BindLua, SetRotation),
		lunamethod(SpriteAnim_BindLua, SetOpacity),
		lunamethod(SpriteAnim_BindLua, SetFade),
		lunamethod(SpriteAnim_BindLua, SetWobbleAnimAmount),
		lunamethod(SpriteAnim_BindLua, SetWobbleAnimSpeed),
		lunamethod(SpriteAnim_BindLua, SetRepeatable),
		lunamethod(SpriteAnim_BindLua, SetVelocity),
		lunamethod(SpriteAnim_BindLua, SetScaleX),
		lunamethod(SpriteAnim_BindLua, SetScaleY),
		lunamethod(SpriteAnim_BindLua, SetMovingTexAnim),
		lunamethod(SpriteAnim_BindLua, SetDrawRecAnim),

		lunamethod(SpriteAnim_BindLua, GetRot),
		lunamethod(SpriteAnim_BindLua, GetRotation),
		lunamethod(SpriteAnim_BindLua, GetOpacity),
		lunamethod(SpriteAnim_BindLua, GetFade),
		lunamethod(SpriteAnim_BindLua, GetRepeatable),
		lunamethod(SpriteAnim_BindLua, GetVelocity),
		lunamethod(SpriteAnim_BindLua, GetScaleX),
		lunamethod(SpriteAnim_BindLua, GetScaleY),
		lunamethod(SpriteAnim_BindLua, GetMovingTexAnim),
		lunamethod(SpriteAnim_BindLua, GetDrawRecAnim),
		{ nullptr, nullptr }
	};
	Luna<SpriteAnim_BindLua>::PropertyType SpriteAnim_BindLua::properties[] = {
		lunaproperty(SpriteAnim_BindLua, Rot),
		lunaproperty(SpriteAnim_BindLua, Rotation),
		lunaproperty(SpriteAnim_BindLua, Opacity),
		lunaproperty(SpriteAnim_BindLua, Fade),
		lunaproperty(SpriteAnim_BindLua, Repeatable),
		lunaproperty(SpriteAnim_BindLua, Velocity),
		lunaproperty(SpriteAnim_BindLua, ScaleX),
		lunaproperty(SpriteAnim_BindLua, ScaleY),
		lunaproperty(SpriteAnim_BindLua, MovingTexAnim),
		lunaproperty(SpriteAnim_BindLua, DrawRecAnim),
		{ nullptr, nullptr }
	};

	SpriteAnim_BindLua::SpriteAnim_BindLua(const Protean::Sprite::Anim& anim) :anim(anim)
	{
	}

	SpriteAnim_BindLua::SpriteAnim_BindLua(lua_State* L)
	{
		anim = Protean::Sprite::Anim();
	}

	int SpriteAnim_BindLua::SetRot(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.rot = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetRot(float rot) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetRotation(lua_State* L)
	{
		return SetRot(L);
	}
	int SpriteAnim_BindLua::SetOpacity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.opa = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetOpacity(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetFade(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.fad = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetFade(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetWobbleAnimAmount(lua_State* L) 
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.wobbleAnim.amount = XMFLOAT2(Protean::Lua::SGetFloat(L, 1), Protean::Lua::SGetFloat(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetWobbleAnimAmount(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetWobbleAnimSpeed(lua_State* L) 
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.wobbleAnim.speed = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetWobbleAnimSpeed(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetRepeatable(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.repeatable = Protean::Lua::SGetBool(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetRepeatable(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec != nullptr)
			{
				XMStoreFloat3(&anim.vel, XMLoadFloat4(&vec->data));
			}
			else
			{
				Protean::Lua::SError(L, "SetVelocity(Vector val) argument is not a Vector!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetVelocity(Vector val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetScaleX(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.scaleX = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetScaleX(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetScaleY(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.scaleY = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetScaleY(float val) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetMovingTexAnim(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			MovingTexAnim_BindLua* data = Luna<MovingTexAnim_BindLua>::lightcheck(L, 1);
			if (data != nullptr)
			{
				anim.movingTexAnim = data->anim;
			}
			else
			{
				Protean::Lua::SError(L, "SetMovingTexAnim(MovingTexAnim data) argument is not a MovingTexAnim!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetMovingTexAnim(MovingTexAnim data) not enough arguments!");
		}
		return 0;
	}
	int SpriteAnim_BindLua::SetDrawRecAnim(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			DrawRectAnim_BindLua* data = Luna<DrawRectAnim_BindLua>::lightcheck(L, 1);
			if (data != nullptr)
			{
				anim.drawRectAnim = data->anim;
			}
			else
			{
				Protean::Lua::SError(L, "SetDrawRecAnim(DrawRecAnim data) argument is not a DrawRecAnim!");
			}
		}
		else
		{
			Protean::Lua::SError(L, "SetDrawRecAnim(DrawRecAnim data) not enough arguments!");
		}
		return 0;
	}

	int SpriteAnim_BindLua::GetRot(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.rot);
		return 1;
	}
	int SpriteAnim_BindLua::GetRotation(lua_State* L)
	{
		return GetRot(L);
	}
	int SpriteAnim_BindLua::GetOpacity(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.opa);
		return 1;
	}
	int SpriteAnim_BindLua::GetFade(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.fad);
		return 1;
	}
	int SpriteAnim_BindLua::GetRepeatable(lua_State* L)
	{
		Protean::Lua::SSetBool(L, anim.repeatable);
		return 1;
	}
	int SpriteAnim_BindLua::GetVelocity(lua_State* L)
	{
		Luna<Vector_BindLua>::push(L, XMLoadFloat3(&anim.vel));
		return 1;
	}
	int SpriteAnim_BindLua::GetScaleX(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.scaleX);
		return 1;
	}
	int SpriteAnim_BindLua::GetScaleY(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.scaleY);
		return 1;
	}
	int SpriteAnim_BindLua::GetMovingTexAnim(lua_State* L)
	{
		Luna<MovingTexAnim_BindLua>::push(L, anim.movingTexAnim);
		return 1;
	}
	int SpriteAnim_BindLua::GetDrawRecAnim(lua_State* L)
	{
		Luna<DrawRectAnim_BindLua>::push(L, anim.drawRectAnim);
		return 1;
	}

	void SpriteAnim_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<SpriteAnim_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<MovingTexAnim_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<DrawRectAnim_BindLua>::Register(Protean::Lua::GetLuaState());
		}
	}




	Luna<MovingTexAnim_BindLua>::FunctionType MovingTexAnim_BindLua::methods[] = {
		lunamethod(MovingTexAnim_BindLua, SetSpeedX),
		lunamethod(MovingTexAnim_BindLua, SetSpeedY),

		lunamethod(MovingTexAnim_BindLua, GetSpeedX),
		lunamethod(MovingTexAnim_BindLua, GetSpeedY),
		{ nullptr, nullptr }
	};
	Luna<MovingTexAnim_BindLua>::PropertyType MovingTexAnim_BindLua::properties[] = {
		lunaproperty(MovingTexAnim_BindLua, SpeedX),
		lunaproperty(MovingTexAnim_BindLua, SpeedY),
		{ nullptr, nullptr }
	};

	MovingTexAnim_BindLua::MovingTexAnim_BindLua(const Protean::Sprite::Anim::MovingTexAnim& anim) :anim(anim)
	{
	}

	MovingTexAnim_BindLua::MovingTexAnim_BindLua(lua_State* L)
	{
		anim = Protean::Sprite::Anim::MovingTexAnim();
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.speedX = Protean::Lua::SGetFloat(L, 1);
			if (argc > 1)
			{
				anim.speedY = Protean::Lua::SGetFloat(L, 2);
			}
		}
	}

	MovingTexAnim_BindLua::~MovingTexAnim_BindLua()
	{
	}

	int MovingTexAnim_BindLua::SetSpeedX(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.speedX = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetSpeedX(float val) not enough arguments!");
		}
		return 0;
	}
	int MovingTexAnim_BindLua::SetSpeedY(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.speedY = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetSpeedY(float val) not enough arguments!");
		}
		return 0;
	}

	int MovingTexAnim_BindLua::GetSpeedX(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.speedX);
		return 1;
	}
	int MovingTexAnim_BindLua::GetSpeedY(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.speedY);
		return 1;
	}



	Luna<DrawRectAnim_BindLua>::FunctionType DrawRectAnim_BindLua::methods[] = {
		lunamethod(DrawRectAnim_BindLua, SetFrameRate),
		lunamethod(DrawRectAnim_BindLua, SetFrameCount),
		lunamethod(DrawRectAnim_BindLua, SetHorizontalFrameCount),

		lunamethod(DrawRectAnim_BindLua, GetFrameRate),
		lunamethod(DrawRectAnim_BindLua, GetFrameCount),
		lunamethod(DrawRectAnim_BindLua, GetHorizontalFrameCount),
		{ NULL, NULL }
	};
	Luna<DrawRectAnim_BindLua>::PropertyType DrawRectAnim_BindLua::properties[] = {
		lunaproperty(DrawRectAnim_BindLua, FrameRate),
		lunaproperty(DrawRectAnim_BindLua, FrameCount),
		lunaproperty(DrawRectAnim_BindLua, HorizontalFrameCount),
		{ NULL, NULL }
	};

	DrawRectAnim_BindLua::DrawRectAnim_BindLua(const Protean::Sprite::Anim::DrawRectAnim& anim) :anim(anim)
	{
	}

	DrawRectAnim_BindLua::DrawRectAnim_BindLua(lua_State* L)
	{
		anim = Protean::Sprite::Anim::DrawRectAnim();
	}

	DrawRectAnim_BindLua::~DrawRectAnim_BindLua()
	{
	}

	int DrawRectAnim_BindLua::SetFrameRate(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.frameRate = Protean::Lua::SGetFloat(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetFrameRate(float val) not enough arguments!");
		}
		return 0;
	}
	int DrawRectAnim_BindLua::SetFrameCount(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.frameCount = Protean::Lua::SGetInt(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetFrameCount(int val) not enough arguments!");
		}
		return 0;
	}
	int DrawRectAnim_BindLua::SetHorizontalFrameCount(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			anim.horizontalFrameCount = Protean::Lua::SGetInt(L, 1);
		}
		else
		{
			Protean::Lua::SError(L, "SetHorizontalFrameCount(int val) not enough arguments!");
		}
		return 0;
	}

	int DrawRectAnim_BindLua::GetFrameRate(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, anim.frameRate);
		return 1;
	}
	int DrawRectAnim_BindLua::GetFrameCount(lua_State* L)
	{
		Protean::Lua::SSetInt(L, anim.frameCount);
		return 1;
	}
	int DrawRectAnim_BindLua::GetHorizontalFrameCount(lua_State* L)
	{
		Protean::Lua::SSetInt(L, anim.horizontalFrameCount);
		return 1;
	}

}

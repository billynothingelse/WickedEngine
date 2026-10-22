#include "script/bindings/RenderPath3DBindLua.h"
#include "script/bindings/TextureBindLua.h"

namespace Protean::Lua
{

	Luna<RenderPath3D_BindLua>::FunctionType RenderPath3D_BindLua::methods[] = {
		lunamethod(RenderPath2D_BindLua, AddSprite),
		lunamethod(RenderPath2D_BindLua, AddFont),
		lunamethod(RenderPath2D_BindLua, RemoveSprite),
		lunamethod(RenderPath2D_BindLua, RemoveFont),
		lunamethod(RenderPath2D_BindLua, ClearSprites),
		lunamethod(RenderPath2D_BindLua, ClearFonts),
		lunamethod(RenderPath2D_BindLua, GetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, GetFontOrder),

		lunamethod(RenderPath2D_BindLua, AddLayer),
		lunamethod(RenderPath2D_BindLua, GetLayers),
		lunamethod(RenderPath2D_BindLua, SetLayerOrder),
		lunamethod(RenderPath2D_BindLua, SetSpriteOrder),
		lunamethod(RenderPath2D_BindLua, SetFontOrder),
		lunamethod(RenderPath2D_BindLua, GetHDRScaling),
		lunamethod(RenderPath2D_BindLua, SetHDRScaling),

		lunamethod(RenderPath_BindLua, GetLayerMask),
		lunamethod(RenderPath_BindLua, SetLayerMask),

		lunamethod(RenderPath3D_BindLua, SetResolutionScale),
		lunamethod(RenderPath3D_BindLua, SetAO),
		lunamethod(RenderPath3D_BindLua, SetAOPower),
		lunamethod(RenderPath3D_BindLua, SetAORange),
		lunamethod(RenderPath3D_BindLua, SetSSREnabled),
		lunamethod(RenderPath3D_BindLua, SetSSGIEnabled),
		lunamethod(RenderPath3D_BindLua, SetRaytracedDiffuseEnabled),
		lunamethod(RenderPath3D_BindLua, SetRaytracedReflectionsEnabled),
		lunamethod(RenderPath3D_BindLua, SetShadowsEnabled),
		lunamethod(RenderPath3D_BindLua, SetReflectionsEnabled),
		lunamethod(RenderPath3D_BindLua, SetPlanarReflectionQuality),
		lunamethod(RenderPath3D_BindLua, SetFXAAEnabled),
		lunamethod(RenderPath3D_BindLua, SetBloomEnabled),
		lunamethod(RenderPath3D_BindLua, SetBloomThreshold),
		lunamethod(RenderPath3D_BindLua, SetColorGradingEnabled),
		lunamethod(RenderPath3D_BindLua, SetVolumeLightsEnabled),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsEnabled),
		lunamethod(RenderPath3D_BindLua, SetLensFlareEnabled),
		lunamethod(RenderPath3D_BindLua, SetMotionBlurEnabled),
		lunamethod(RenderPath3D_BindLua, SetDitherEnabled),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldEnabled),
		lunamethod(RenderPath3D_BindLua, SetEyeAdaptionEnabled),
		lunamethod(RenderPath3D_BindLua, SetMSAASampleCount),
		lunamethod(RenderPath3D_BindLua, SetCRTFilterEnabled),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterEnabled),
		lunamethod(RenderPath3D_BindLua, SetSharpenFilterAmount),
		lunamethod(RenderPath3D_BindLua, SetExposure),
		lunamethod(RenderPath3D_BindLua, SetHDRCalibration),
		lunamethod(RenderPath3D_BindLua, SetMotionBlurStrength),
		lunamethod(RenderPath3D_BindLua, SetDepthOfFieldStrength),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsStrength),
		lunamethod(RenderPath3D_BindLua, SetOutlineEnabled),
		lunamethod(RenderPath3D_BindLua, SetOutlineThickness),
		lunamethod(RenderPath3D_BindLua, SetOutlineThreshold),
		lunamethod(RenderPath3D_BindLua, SetOutlineColor),
		lunamethod(RenderPath3D_BindLua, SetFSREnabled),
		lunamethod(RenderPath3D_BindLua, SetFSRSharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Enabled),
		lunamethod(RenderPath3D_BindLua, SetFSR2Sharpness),
		lunamethod(RenderPath3D_BindLua, SetFSR2Preset),
		lunamethod(RenderPath3D_BindLua, SetTonemap),
		lunamethod(RenderPath3D_BindLua, SetVisibilityComputeShadingEnabled),

		lunamethod(RenderPath3D_BindLua, SetCropLeft),
		lunamethod(RenderPath3D_BindLua, SetCropTop),
		lunamethod(RenderPath3D_BindLua, SetCropRight),
		lunamethod(RenderPath3D_BindLua, SetCropBottom),

		lunamethod(RenderPath3D_BindLua, GetLastPostProcessRT),

		lunamethod(RenderPath3D_BindLua, SetDistortionOverlay),
		lunamethod(RenderPath3D_BindLua, SetChromaticAberrationEnabled),
		lunamethod(RenderPath3D_BindLua, SetChromaticAberrationAmount),
		lunamethod(RenderPath3D_BindLua, SetEyeAdaptionRate),
		lunamethod(RenderPath3D_BindLua, SetEyeAdaptionKey),
		lunamethod(RenderPath3D_BindLua, SetContrast),
		lunamethod(RenderPath3D_BindLua, SetSaturation),
		lunamethod(RenderPath3D_BindLua, SetBrightness),
		lunamethod(RenderPath3D_BindLua, SetLightShaftsFadeSpeed),
		lunamethod(RenderPath3D_BindLua, SetMeshBlendEnabled),
		lunamethod(RenderPath3D_BindLua, SetOcclusionCullingEnabled),
		lunamethod(RenderPath3D_BindLua, SetSSGIDepthRejection),

		lunamethod(RenderPath2D_BindLua, CopyFrom),
		{ NULL, NULL }
	};
	Luna<RenderPath3D_BindLua>::PropertyType RenderPath3D_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int RenderPath3D_BindLua::SetResolutionScale(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetResolutionScale(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			float value = Protean::Lua::SGetFloat(L, 1);
			((RenderPath3D*)component)->resolutionScale = value;
		}
		else
			Protean::Lua::SError(L, "SetResolutionScale(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAO(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetAO(AO value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			int value = Protean::Lua::SGetInt(L, 1);
			RenderPath3D::AO ao = (RenderPath3D::AO)value;
			((RenderPath3D*)component)->setAO(ao);
		}
		else
			Protean::Lua::SError(L, "SetAO(AO value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAOPower(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetAOPower(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			float value = Protean::Lua::SGetFloat(L, 1);
			((RenderPath3D*)component)->setAOPower(value);
		}
		else
			Protean::Lua::SError(L, "SetAOPower(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetAORange(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetAORange(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			float value = Protean::Lua::SGetFloat(L, 1);
			((RenderPath3D*)component)->setAORange(value);
		}
		else
			Protean::Lua::SError(L, "SetAORange(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSSREnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSREnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetSSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSSGIEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSSGIEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setSSGIEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetSSGIEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedDiffuseEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetRaytracedDiffuseEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedDiffuseEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetRaytracedDiffuseEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetRaytracedReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetRaytracedReflectionsEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setRaytracedReflectionsEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetRaytracedReflectionsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetShadowsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetShadowsEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setShadowsEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetShadowsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetReflectionsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetReflectionsEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setReflectionsEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetReflectionsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetPlanarReflectionQuality(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetPlanarReflectionQuality(float resolutionScale, int msaaSampleCount) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 1)
			((RenderPath3D*)component)->setPlanarReflectionQuality(Protean::Lua::SGetFloat(L, 1), Protean::Lua::SGetInt(L, 2));
		else
			Protean::Lua::SError(L, "SetPlanarReflectionQuality(float resolutionScale, int msaaSampleCount) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFXAAEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFXAAEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFXAAEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetFXAAEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetBloomEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setBloomEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetBloomEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetBloomThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetBloomThreshold(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setBloomThreshold(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetBloomThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetColorGradingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetColorGradingEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setColorGradingEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetColorGradingEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetVolumeLightsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetVolumeLightsEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setVolumeLightsEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetVolumeLightsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLightShaftsEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLightShaftsEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetLightShaftsEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLensFlareEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLensFlareEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setLensFlareEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetLensFlareEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetMotionBlurEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setMotionBlurEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetMotionBlurEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDitherEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetDitherEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setDitherEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetDitherEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetDepthOfFieldEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetDepthOfFieldEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetEyeAdaptionEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetEyeAdaptionEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setEyeAdaptionEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetEyeAdaptionEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMSAASampleCount(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetMSAASampleCount(int value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMSAASampleCount((uint32_t)Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetMSAASampleCount(int value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetCRTFilterEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetCRTFilterEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setCRTFilterEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetCRTFilterEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSharpenFilterEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetSharpenFilterEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetSharpenFilterAmount(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSharpenFilterAmount(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSharpenFilterAmount(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetSharpenFilterAmount(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetExposure(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetExposure(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setExposure(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetExposure(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetHDRCalibration(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetHDRCalibration(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setHDRCalibration(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetHDRCalibration(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetMotionBlurStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetMotionBlurStrength(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMotionBlurStrength(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetMotionBlurStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetDepthOfFieldStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetDepthOfFieldStrength(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setDepthOfFieldStrength(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetDepthOfFieldStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetLightShaftsStrength(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLightShaftsStrength(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setLightShaftsStrength(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetLightShaftsStrength(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetOutlineEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetOutlineEnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThickness(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetOutlineThickness(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThickness(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetOutlineThickness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineThreshold(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetOutlineThreshold(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOutlineThreshold(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetOutlineThreshold(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetOutlineColor(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetOutlineColor(float r,g,b,a) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 3)
		{
			((RenderPath3D*)component)->setOutlineColor(Protean::Lua::SGetFloat4(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetOutlineColor(float r,g,b,a) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSREnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFSREnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSREnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetFSREnabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSRSharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFSRSharpness(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSRSharpness(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFSRSharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Enabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFSR2Enabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setFSR2Enabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetFSR2Enabled(bool value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Sharpness(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFSR2Sharpness(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Sharpness(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFSR2Sharpness(float value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetFSR2Preset(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetFSR2Preset(FSR2_Preset value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setFSR2Preset((Protean::RenderPath3D::FSR2_Preset)Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFSR2Preset(FSR2_Preset value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetTonemap(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetTonemap(Tonemap value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setTonemap((Protean::Renderer::Tonemap)Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetTonemap(Tonemap value) not enough arguments!");
		return 0;
	}
	int RenderPath3D_BindLua::SetCropLeft(lua_State* L)
	{
		((RenderPath3D*)component)->crop_left = Protean::Lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropTop(lua_State* L)
	{
		((RenderPath3D*)component)->crop_top = Protean::Lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropRight(lua_State* L)
	{
		((RenderPath3D*)component)->crop_right = Protean::Lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetCropBottom(lua_State* L)
	{
		((RenderPath3D*)component)->crop_bottom = Protean::Lua::SGetFloat(L, 1);
		return 0;
	}
	int RenderPath3D_BindLua::SetVisibilityComputeShadingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetVisibilityComputeShadingEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
			((RenderPath3D*)component)->setVisibilityComputeShadingEnabled(Protean::Lua::SGetBool(L, 1));
		else
			Protean::Lua::SError(L, "SetVisibilityComputeShadingEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::GetLastPostProcessRT(lua_State* L)
	{
		const Protean::Graphics::Texture* tex = ((RenderPath3D*)component)->GetLastPostprocessRT();
		if (tex == nullptr)
			return 0;
		Luna<Texture_BindLua>::push(L, *tex);
		return 1;
	}

	int RenderPath3D_BindLua::SetDistortionOverlay(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "SetDistortionOverlay(Texture texture) not enough arguments!");
			return 0;
		}
		Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
		if (tex == nullptr)
		{
			Protean::Lua::SError(L, "SetDistortionOverlay(Texture texture) first argument is not a Texture!");
			return 0;
		}
		if (tex->resource.IsValid())
		{
			((RenderPath3D*)component)->distortion_overlay = tex->resource.GetTexture();
		}
		else
		{
			((RenderPath3D*)component)->distortion_overlay = {};
		}
		return 0;
	}

	int RenderPath3D_BindLua::SetChromaticAberrationEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetChromaticAberrationEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setChromaticAberrationEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetChromaticAberrationEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetChromaticAberrationAmount(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetChromaticAberrationAmount(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setChromaticAberrationAmount(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetChromaticAberrationAmount(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetEyeAdaptionRate(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetEyeAdaptionRate(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setEyeAdaptionRate(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetEyeAdaptionRate(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetEyeAdaptionKey(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetEyeAdaptionKey(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setEyeAdaptionKey(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetEyeAdaptionKey(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetContrast(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetContrast(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setContrast(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetContrast(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetSaturation(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSaturation(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSaturation(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetSaturation(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetBrightness(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetBrightness(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setBrightness(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetBrightness(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetLightShaftsFadeSpeed(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetLightShaftsFadeSpeed(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setLightShaftsFadeSpeed(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetLightShaftsFadeSpeed(float value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetMeshBlendEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetMeshBlendEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setMeshBlendEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetMeshBlendEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetOcclusionCullingEnabled(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetOcclusionCullingEnabled(bool value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setOcclusionCullingEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetOcclusionCullingEnabled(bool value) not enough arguments!");
		return 0;
	}

	int RenderPath3D_BindLua::SetSSGIDepthRejection(lua_State* L)
	{
		if (component == nullptr)
		{
			Protean::Lua::SError(L, "SetSSGIDepthRejection(float value) component is null!");
			return 0;
		}
		if (Protean::Lua::SGetArgCount(L) > 0)
		{
			((RenderPath3D*)component)->setSSGIDepthRejection(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetSSGIDepthRejection(float value) not enough arguments!");
		return 0;
	}

static const std::string value_bindings = R"(
AO_DISABLED = 0
AO_SSAO = 1
AO_HBAO = 2
AO_MSAO = 3
AO_RTAO = 4

FSR2_Preset = {
	Quality = 0,
	Balanced = 1,
	Performance = 2,
	Ultra_Performance = 3,
}

Tonemap = {
	Reinhard = 0,
	ACES = 1,
}
)";

	void RenderPath3D_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<RenderPath3D_BindLua>::Register(Protean::Lua::GetLuaState());

			Protean::Lua::RunText(value_bindings);
		}
	}

}

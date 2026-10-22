#include "script/bindings/RendererBindLua.h"
#include "render/Renderer.h"
#include "core/Helper.h"
#include "scene/Scene.h"
#include "script/bindings/SceneBindLua.h"
#include "script/bindings/MathBindLua.h"
#include "script/bindings/TextureBindLua.h"
#include "render/EmittedParticle.h"
#include "render/HairParticle.h"
#include "script/bindings/PrimitiveBindLua.h"
#include "core/EventHandler.h"
#include "script/bindings/VoxelGridBindLua.h"
#include "script/bindings/PathQueryBindLua.h"
#include "script/bindings/TrailRendererBindLua.h"

using namespace Protean;
using namespace Protean::Graphics;
using namespace Protean;
using namespace Protean::Lua::SceneBindings;
using namespace Protean::Lua::PrimitiveBindings;

namespace Protean::Lua::RendererBindings
{
	int SetGamma(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Lua::SSetString(L, "SetGamma() no longer supported!");
		}
		else
		{
			Protean::Lua::SError(L, "SetGamma(float) not enough arguments!");
		}
		return 0;
	}
	int SetGameSpeed(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetGameSpeed(Protean::Lua::SGetFloat(L, 1));
		}
		else
		{
			Protean::Lua::SError(L,"SetGameSpeed(float) not enough arguments!");
		}
		return 0;
	}
	int GetGameSpeed(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, Protean::Renderer::GetGameSpeed());
		return 1;
	}
	int IsRaytracingSupported(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Graphics::GetDevice()->CheckCapability(GraphicsDeviceCapability::RAYTRACING));
		return 1;
	}

	int SetShadowProps2D(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetShadowProps2D(Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetShadowProps2D(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetShadowPropsCube(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetShadowPropsCube(Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetShadowPropsCube(int max_resolution) not enough arguments!");
		return 0;
	}
	int SetDebugPartitionTreeEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugPartitionTree(Protean::Lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugBoxesEnabled(lua_State* L)
	{
		Protean::Lua::SError(L, "SetDebugBoxesEnabled is obsolete! Use SetDebugPartitionTreeEnabled(bool value) instead to draw a partition tree!");
		return 0;
	}
	int SetDebugBonesEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugBoneLines(Protean::Lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEmittersEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugEmitters(Protean::Lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetDebugEnvProbesEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugEnvProbes(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugEnvProbesEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugForceFieldsEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugForceFields(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugForceFieldsEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCamerasEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugCameras(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugCamerasEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDebugCollidersEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawDebugColliders(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugCollidersEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetGridHelperEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetToDrawGridHelper(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetGridHelperEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDDGIDebugEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetDDGIDebugEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDDGIDebugEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetDDGIEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetDDGIEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDDGIEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetVSyncEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::EventHandler::SetVSync(Protean::Lua::SGetBool(L, 1));
		}
		return 0;
	}
	int SetResolution(lua_State* L)
	{
		Protean::Lua::SError(L, "SetResolution() is deprecated, now it's handled by window events!");
		return 0;
	}
	int SetDebugLightCulling(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetDebugLightCulling(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugLightCulling(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetOcclusionCullingEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetOcclusionCullingEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetOcclusionCullingEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetTemporalAAEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetTemporalAAEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetTemporalAAEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetRaytracedShadowsEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetRaytracedShadowsEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetRaytracedShadowsEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetMeshShaderAllowed(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetMeshShaderAllowed(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetMeshShaderAllowed(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetMeshletOcclusionCullingEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetMeshletOcclusionCullingEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetMeshletOcclusionCullingEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetCapsuleShadowEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetCapsuleShadowEnabled(bool enabled) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowFade(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetCapsuleShadowFade(Protean::Lua::SGetFloat(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetCapsuleShadowFade(float value) not enough arguments!");
		}
		return 0;
	}
	int SetCapsuleShadowAngle(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetCapsuleShadowAngle(Protean::Lua::SGetFloat(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetCapsuleShadowAngle(float value) not enough arguments!");
		}
		return 0;
	}
	int SetShadowLODOverrideEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetShadowLODOverrideEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetShadowLODOverrideEnabled(bool value) not enough arguments!");
		}
		return 0;
	}
	int SetDebugDrawEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Renderer::SetDebugDrawEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
		{
			Protean::Lua::SError(L, "SetDebugDrawEnabled(bool value) not enough arguments!");
		}
		return 0;
	}

	int DrawLine(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			Vector_BindLua* b = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (a && b)
			{
				Protean::Renderer::RenderableLine line;
				XMStoreFloat3(&line.start, XMLoadFloat4(&a->data));
				XMStoreFloat3(&line.end, XMLoadFloat4(&b->data));
				bool depth = false;
				if (argc > 2)
				{
					Vector_BindLua* c = Luna<Vector_BindLua>::lightcheck(L, 3);
					if (c)
					{
						XMStoreFloat4(&line.color_start, XMLoadFloat4(&c->data));
						XMStoreFloat4(&line.color_end, XMLoadFloat4(&c->data));
					}
					else
						Protean::Lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");

					if (argc > 3)
					{
						depth = Protean::Lua::SGetBool(L, 4);
					}
				}
				Protean::Renderer::DrawLine(line, depth);
			}
			else
				Protean::Lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) one or more arguments are not vectors!");
		}
		else
			Protean::Lua::SError(L, "DrawLine(Vector origin,end, opt Vector color, opt bool depth = false) not enough arguments!");

		return 0;
	}
	int DrawPoint(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Vector_BindLua* a = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (a)
			{
				Protean::Renderer::RenderablePoint point;
				XMStoreFloat3(&point.position, XMLoadFloat4(&a->data));
				bool depth = false;
				if (argc > 1)
				{
					point.size = Protean::Lua::SGetFloat(L, 2);

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color)
						{
							point.color = color->data;
						}

						if (argc > 3)
						{
							depth = Protean::Lua::SGetBool(L, 4);
						}
					}
				}
				Protean::Renderer::DrawPoint(point, depth);
			}
			else
				Protean::Lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) first argument must be a Vector type!");
		}
		else
			Protean::Lua::SError(L, "DrawPoint(Vector origin, opt float size, opt Vector color, opt bool depth = false) not enough arguments!");

		return 0;
	}
	int DrawBox(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Matrix_BindLua* m = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (m)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Protean::Lua::SGetBool(L, 3);
						}
						Protean::Renderer::DrawBox(m->data, color->data, depth);
						return 0;
					}
				}

				Protean::Renderer::DrawBox(m->data);
			}
			else
				Protean::Lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Protean::Lua::SError(L, "DrawBox(Matrix boxMatrix, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawSphere(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Sphere_BindLua* sphere = Luna<Sphere_BindLua>::lightcheck(L, 1);
			if (sphere)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Protean::Lua::SGetBool(L, 3);
						}
						Protean::Renderer::DrawSphere(sphere->sphere, color->data, depth);
						return 0;
					}
				}

				Protean::Renderer::DrawSphere(sphere->sphere);
			}
			else
				Protean::Lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Protean::Lua::SError(L, "DrawSphere(Sphere sphere, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawCapsule(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Capsule_BindLua* capsule = Luna<Capsule_BindLua>::lightcheck(L, 1);
			if (capsule)
			{
				if (argc > 1)
				{
					Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 2);
					if (color)
					{
						bool depth = true;
						if (argc > 2)
						{
							depth = Protean::Lua::SGetBool(L, 3);
						}
						Protean::Renderer::DrawCapsule(capsule->capsule, color->data, depth);
						return 0;
					}
				}

				Protean::Renderer::DrawCapsule(capsule->capsule);
			}
			else
				Protean::Lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) first argument must be a Matrix type!");
		}
		else
			Protean::Lua::SError(L, "DrawCapsule(Capsule capsule, opt Vector color, opt bool depth = true) not enough arguments!");

		return 0;
	}
	int DrawDebugText(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			std::string text = Protean::Lua::SGetString(L, 1);
			Protean::Renderer::DebugTextParams params;
			if (argc > 1)
			{
				Vector_BindLua* position = Luna<Vector_BindLua>::lightcheck(L, 2);
				if (position != nullptr)
				{
					params.position.x = position->data.x;
					params.position.y = position->data.y;
					params.position.z = position->data.z;

					if (argc > 2)
					{
						Vector_BindLua* color = Luna<Vector_BindLua>::lightcheck(L, 3);
						if (color != nullptr)
						{
							params.color = color->data;

							if (argc > 3)
							{
								params.scaling = Protean::Lua::SGetFloat(L, 4);

								if (argc > 4)
								{
									params.flags = Protean::Lua::SGetInt(L, 5);
								}
							}
						}
						else
							Protean::Lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) third argument was not a Vector!");
					}
				}
				else
					Protean::Lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) second argument was not a Vector!");

			}
			Protean::Renderer::DrawDebugText(text.c_str(), params);
		}
		else
			Protean::Lua::SError(L, "DrawDebugText(string text, opt Vector position, opt Vector color, opt float scaling, opt int flags) not enough arguments!");

		return 0;
	}
	int DrawVoxelGrid(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			VoxelGrid_BindLua* a = Luna<VoxelGrid_BindLua>::lightcheck(L, 1);
			if (a)
			{
				Protean::Renderer::DrawVoxelGrid(a->voxelgrid);
			}
			else
				Protean::Lua::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) first argument must be a VoxelGrid type!");
		}
		else
			Protean::Lua::SError(L, "DrawVoxelGrid(VoxelGrid voxelgrid) not enough arguments!");

		return 0;
	}
	int DrawPathQuery(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			PathQuery_BindLua* a = Luna<PathQuery_BindLua>::lightcheck(L, 1);
			if (a)
			{
				Protean::Renderer::DrawPathQuery(a->pathquery);
			}
			else
				Protean::Lua::SError(L, "DrawPathQuery(PathQuery pathquery) first argument must be a PathQuery type!");
		}
		else
			Protean::Lua::SError(L, "DrawPathQuery(PathQuery pathquery) not enough arguments!");

		return 0;
	}
	int DrawTrail(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			TrailRenderer_BindLua* a = Luna<TrailRenderer_BindLua>::lightcheck(L, 1);
			if (a)
			{
				Protean::Renderer::DrawTrail(&a->trail);
			}
			else
				Protean::Lua::SError(L, "DrawTrail(TrailRenderer trail) first argument must be a TrailRenderer type!");
		}
		else
			Protean::Lua::SError(L, "DrawTrail(TrailRenderer trail) not enough arguments!");

		return 0;
	}

	class PaintTextureParams_BindLua
	{
	public:
		Protean::Renderer::PaintTextureParams params;

		PaintTextureParams_BindLua(const Protean::Renderer::PaintTextureParams& params) : params(params) {}
		PaintTextureParams_BindLua(lua_State* L) {}

		int SetEditTexture(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetEditTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Protean::Lua::SError(L, "SetEditTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.editTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetBrushTexture(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Protean::Lua::SError(L, "SetBrushTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.brushTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetRevealTexture(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetRevealTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Protean::Lua::SError(L, "SetRevealTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.revealTex = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetCenterPixel(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetCenterPixel(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetCenterPixel(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushCenter.x = uint32_t(vec->data.x);
			params.push.xPaintBrushCenter.y = uint32_t(vec->data.y);
			return 0;
		}
		int SetBrushColor(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushColor(Vector value): not enough arguments!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetBrushColor(Vector value): argument is not a Vector!");
				return 0;
			}
			params.push.xPaintBrushColor = Protean::Color::fromFloat4(vec->data);
			return 0;
		}
		int SetBrushRadius(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushRadius(int value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRadius = Protean::Lua::SGetInt(L, 1);
			return 0;
		}
		int SetBrushAmount(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushAmount(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushAmount = Protean::Lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushSmoothness(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushSmoothness(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushSmoothness = Protean::Lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushRotation(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushRotation(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushRotation = Protean::Lua::SGetFloat(L, 1);
			return 0;
		}
		int SetBrushShape(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetBrushShape(float value): not enough arguments!");
				return 0;
			}
			params.push.xPaintBrushShape = Protean::Lua::SGetInt(L, 1);
			return 0;
		}

		inline static constexpr char className[] = "PaintTextureParams";
		inline static constexpr Luna<PaintTextureParams_BindLua>::FunctionType methods[] = {
			lunamethod(PaintTextureParams_BindLua, SetEditTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushTexture),
			lunamethod(PaintTextureParams_BindLua, SetRevealTexture),
			lunamethod(PaintTextureParams_BindLua, SetBrushColor),
			lunamethod(PaintTextureParams_BindLua, SetCenterPixel),
			lunamethod(PaintTextureParams_BindLua, SetBrushRadius),
			lunamethod(PaintTextureParams_BindLua, SetBrushAmount),
			lunamethod(PaintTextureParams_BindLua, SetBrushSmoothness),
			lunamethod(PaintTextureParams_BindLua, SetBrushRotation),
			lunamethod(PaintTextureParams_BindLua, SetBrushShape),
			{ nullptr, nullptr }
		};
		inline static constexpr Luna<PaintTextureParams_BindLua>::PropertyType properties[] = {
			{ nullptr, nullptr }
		};
	};

	int PaintIntoTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "PaintIntoTexture(PaintTextureParams params): not enough arguments!");
			return 0;
		}
		PaintTextureParams_BindLua* params = Luna<PaintTextureParams_BindLua>::lightcheck(L, 1);
		if (params == nullptr)
		{
			Protean::Lua::SError(L, "PaintIntoTexture(PaintTextureParams params): argument is not a PaintTextureParams!");
			return 0;
		}
		Protean::Renderer::PaintIntoTexture(params->params);
		return 0;
	}
	int CreatePaintableTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 2)
		{
			Protean::Lua::SError(L, "CreatePaintableTexture(int width,height, opt int mips = 0, opt Vector initialColor = Vector()): not enough arguments!");
			return 0;
		}
		uint32_t width = (uint32_t)Protean::Lua::SGetInt(L, 1);
		uint32_t height = (uint32_t)Protean::Lua::SGetInt(L, 2);
		uint32_t mips = 0;
		Protean::Color color = Protean::Color::Transparent();
		if (argc > 2)
		{
			mips = (uint32_t)Protean::Lua::SGetInt(L, 3);
			if (argc > 3)
			{
				Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (v != nullptr)
				{
					color = Protean::Color::fromFloat4(v->data);
				}
			}
		}
		Luna<Texture_BindLua>::push(L, Protean::Renderer::CreatePaintableTexture(width, height, mips, color));
		return 1;
	}


	class PaintDecalParams_BindLua
	{
	public:
		Protean::Renderer::PaintDecalParams params;

		PaintDecalParams_BindLua(const Protean::Renderer::PaintDecalParams& params) : params(params) {}
		PaintDecalParams_BindLua(lua_State* L) {}

		int SetInTexture(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetInTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Protean::Lua::SError(L, "SetInTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.in_texture = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetOutTexture(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetOutTexture(Texture tex): not enough arguments!");
				return 0;
			}
			Texture_BindLua* tex = Luna<Texture_BindLua>::lightcheck(L, 1);
			if (tex == nullptr)
			{
				Protean::Lua::SError(L, "SetOutTexture(Texture tex): argument is not a Texture!");
				return 0;
			}
			if (tex->resource.IsValid())
			{
				params.out_texture = tex->resource.GetTexture();
			}
			return 0;
		}
		int SetMatrix(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetMatrix(Matrix mat): not enough arguments!");
				return 0;
			}
			Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, 1);
			if (mat == nullptr)
			{
				Protean::Lua::SError(L, "SetMatrix(Texture mat): argument is not a Texture!");
				return 0;
			}
			params.decalMatrix = mat->data;
			return 0;
		}
		int SetObject(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetObject(Entity entity): not enough arguments!");
				return 0;
			}
			params.objectEntity = (Protean::Entity)Protean::Lua::SGetLongLong(L, 1);
			return 0;
		}
		int SetSlopeBlendPower(lua_State* L)
		{
			int argc = Protean::Lua::SGetArgCount(L);
			if (argc < 1)
			{
				Protean::Lua::SError(L, "SetSlopeBlendPower(float power): not enough arguments!");
				return 0;
			}
			params.slopeBlendPower = Protean::Lua::SGetFloat(L, 1);
			return 0;
		}

		inline static constexpr char className[] = "PaintDecalParams";
		inline static constexpr Luna<PaintDecalParams_BindLua>::FunctionType methods[] = {
			lunamethod(PaintDecalParams_BindLua, SetInTexture),
			lunamethod(PaintDecalParams_BindLua, SetOutTexture),
			lunamethod(PaintDecalParams_BindLua, SetMatrix),
			lunamethod(PaintDecalParams_BindLua, SetObject),
			lunamethod(PaintDecalParams_BindLua, SetSlopeBlendPower),
			{ nullptr, nullptr }
		};
		inline static constexpr Luna<PaintDecalParams_BindLua>::PropertyType properties[] = {
			{ nullptr, nullptr }
		};
	};
	int PaintDecalIntoObjectSpaceTexture(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc < 1)
		{
			Protean::Lua::SError(L, "PaintDecalIntoObjectSpaceTexture(PaintDecalParams params): not enough arguments!");
			return 0;
		}
		PaintDecalParams_BindLua* params = Luna<PaintDecalParams_BindLua>::lightcheck(L, 1);
		if (params == nullptr)
		{
			Protean::Lua::SError(L, "PaintDecalIntoObjectSpaceTexture(PaintDecalParams params): argument is not a PaintDecalParams!");
			return 0;
		}
		Protean::Renderer::PaintDecalIntoObjectSpaceTexture(params->params);
		return 0;
	}

	int PutWaterRipple(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			std::string name = Protean::Lua::SGetString(L, 1);
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(name, v->GetFloat3());
			}
			else
				Protean::Lua::SError(L, "PutWaterRipple(string imagename, Vector position) argument is not a Vector!");
		}
		else if (argc > 0)
		{
			Vector_BindLua* v = Luna<Vector_BindLua>::lightcheck(L, 1);
			if (v)
			{
				GetGlobalScene()->PutWaterRipple(v->GetFloat3());
			}
			else
				Protean::Lua::SError(L, "PutWaterRipple(Vector position) argument is not a Vector!");
		}
		else
		{
			Protean::Lua::SError(L, "PutWaterRipple(Vector position) not enough arguments!");
			Protean::Lua::SError(L, "PutWaterRipple(string imagename, Vector position) not enough arguments!");
		}
		return 0;
	}

	int ClearWorld(lua_State* L)
	{
		Scene_BindLua* scene = Luna<Scene_BindLua>::lightcheck(L, 1);
		if (scene == nullptr)
		{
			Protean::Renderer::ClearWorld(*GetGlobalScene());
		}
		else
		{
			Protean::Renderer::ClearWorld(*scene->scene);
		}
		return 0;
	}
	int ReloadShaders(lua_State* L)
	{
		Protean::Renderer::ReloadShaders();
		return 0;
	}

	void Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;

			Luna<PaintTextureParams_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<PaintDecalParams_BindLua>::Register(Protean::Lua::GetLuaState());

			Protean::Lua::RegisterFunc("SetGamma", SetGamma);
			Protean::Lua::RegisterFunc("SetGameSpeed", SetGameSpeed);
			Protean::Lua::RegisterFunc("GetGameSpeed", GetGameSpeed);
			Protean::Lua::RegisterFunc("IsRaytracingSupported", IsRaytracingSupported);

			Protean::Lua::RegisterFunc("SetShadowProps2D", SetShadowProps2D);
			Protean::Lua::RegisterFunc("SetShadowPropsCube", SetShadowPropsCube);
			Protean::Lua::RegisterFunc("SetDebugBoxesEnabled", SetDebugBoxesEnabled);
			Protean::Lua::RegisterFunc("SetDebugPartitionTreeEnabled", SetDebugPartitionTreeEnabled);
			Protean::Lua::RegisterFunc("SetDebugBonesEnabled", SetDebugBonesEnabled);
			Protean::Lua::RegisterFunc("SetDebugEmittersEnabled", SetDebugEmittersEnabled);
			Protean::Lua::RegisterFunc("SetDebugEnvProbesEnabled", SetDebugEnvProbesEnabled);
			Protean::Lua::RegisterFunc("SetDebugForceFieldsEnabled", SetDebugForceFieldsEnabled);
			Protean::Lua::RegisterFunc("SetDebugCamerasEnabled", SetDebugCamerasEnabled);
			Protean::Lua::RegisterFunc("SetDebugCollidersEnabled", SetDebugCollidersEnabled);
			Protean::Lua::RegisterFunc("SetGridHelperEnabled", SetGridHelperEnabled);
			Protean::Lua::RegisterFunc("SetDDGIDebugEnabled", SetDDGIDebugEnabled);
			Protean::Lua::RegisterFunc("SetDDGIEnabled", SetDDGIEnabled);
			Protean::Lua::RegisterFunc("SetVSyncEnabled", SetVSyncEnabled);
			Protean::Lua::RegisterFunc("SetResolution", SetResolution);
			Protean::Lua::RegisterFunc("SetDebugLightCulling", SetDebugLightCulling);
			Protean::Lua::RegisterFunc("SetOcclusionCullingEnabled", SetOcclusionCullingEnabled);
			Protean::Lua::RegisterFunc("SetTemporalAAEnabled", SetTemporalAAEnabled);
			Protean::Lua::RegisterFunc("SetRaytracedShadowsEnabled", SetRaytracedShadowsEnabled);
			Protean::Lua::RegisterFunc("SetMeshShaderAllowed", SetMeshShaderAllowed);
			Protean::Lua::RegisterFunc("SetMeshletOcclusionCullingEnabled", SetMeshletOcclusionCullingEnabled);
			Protean::Lua::RegisterFunc("SetCapsuleShadowEnabled", SetCapsuleShadowEnabled);
			Protean::Lua::RegisterFunc("SetCapsuleShadowFade", SetCapsuleShadowFade);
			Protean::Lua::RegisterFunc("SetCapsuleShadowAngle", SetCapsuleShadowAngle);
			Protean::Lua::RegisterFunc("SetShadowLODOverrideEnabled", SetShadowLODOverrideEnabled);
			Protean::Lua::RegisterFunc("SetDebugDrawEnabled", SetDebugDrawEnabled);

			Protean::Lua::RegisterFunc("DrawLine", DrawLine);
			Protean::Lua::RegisterFunc("DrawPoint", DrawPoint);
			Protean::Lua::RegisterFunc("DrawBox", DrawBox);
			Protean::Lua::RegisterFunc("DrawSphere", DrawSphere);
			Protean::Lua::RegisterFunc("DrawCapsule", DrawCapsule);
			Protean::Lua::RegisterFunc("DrawDebugText", DrawDebugText);
			Protean::Lua::RegisterFunc("DrawVoxelGrid", DrawVoxelGrid);
			Protean::Lua::RegisterFunc("DrawPathQuery", DrawPathQuery);
			Protean::Lua::RegisterFunc("DrawTrail", DrawTrail);

			Protean::Lua::RegisterFunc("PaintIntoTexture", PaintIntoTexture);
			Protean::Lua::RegisterFunc("CreatePaintableTexture", CreatePaintableTexture);
			Protean::Lua::RegisterFunc("PaintDecalIntoObjectSpaceTexture", PaintDecalIntoObjectSpaceTexture);

			Protean::Lua::RegisterFunc("PutWaterRipple", PutWaterRipple);

			Protean::Lua::RegisterFunc("ClearWorld", ClearWorld);
			Protean::Lua::RegisterFunc("ReloadShaders", ReloadShaders);

			Protean::Lua::RunText(R"(
GetScreenWidth = function() return main.GetCanvas().GetLogicalWidth() end
GetScreenHeight = function() return main.GetCanvas().GetLogicalHeight() end

DEBUG_TEXT_DEPTH_TEST = 1
DEBUG_TEXT_CAMERA_FACING = 2
DEBUG_TEXT_CAMERA_SCALING = 4
)");

		}
	}
};

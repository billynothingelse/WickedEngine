#pragma once
#include "script/Lua.h"
#include "script/Luna.h"
#include "math/Primitive.h"

namespace Protean::Lua::PrimitiveBindings
{
	void Bind();


	class Ray_BindLua
	{
	public:
		Protean::Ray ray;

		inline static constexpr char className[] = "Ray";
		static Luna<Ray_BindLua>::FunctionType methods[];
		static Luna<Ray_BindLua>::PropertyType properties[];

		Ray_BindLua(const Protean::Ray& ray) : ray(ray) {}
		Ray_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int GetOrigin(lua_State* L);
		int GetDirection(lua_State* L);
		int SetOrigin(lua_State* L);
		int SetDirection(lua_State* L);
		int CreateFromPoints(lua_State* L);
		int GetPlacementOrientation(lua_State* L);
	};

	class AABB_BindLua
	{
	public:
		Protean::AABB aabb;

		inline static constexpr char className[] = "AABB";
		static Luna<AABB_BindLua>::FunctionType methods[];
		static Luna<AABB_BindLua>::PropertyType properties[];

		AABB_BindLua(const Protean::AABB& aabb) : aabb(aabb) {}
		AABB_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int Intersects2D(lua_State* L);
		int GetMin(lua_State* L);
		int GetMax(lua_State* L);
		int SetMin(lua_State* L);
		int SetMax(lua_State* L);
		int GetCenter(lua_State* L);
		int GetHalfExtents(lua_State* L);
		int Transform(lua_State* L);
		int GetAsBoxMatrix(lua_State* L);
		int ProjectToScreen(lua_State* L);
	};

	class Sphere_BindLua
	{
	public:
		Protean::Sphere sphere;

		inline static constexpr char className[] = "Sphere";
		static Luna<Sphere_BindLua>::FunctionType methods[];
		static Luna<Sphere_BindLua>::PropertyType properties[];

		Sphere_BindLua(const Protean::Sphere& sphere) : sphere(sphere) {}
		Sphere_BindLua(lua_State *L);

		int Intersects(lua_State* L);
		int GetCenter(lua_State* L);
		int GetRadius(lua_State* L);
		int SetCenter(lua_State* L);
		int SetRadius(lua_State* L);
		int GetPlacementOrientation(lua_State* L);
	};

	class Capsule_BindLua
	{
	public:
		Protean::Capsule capsule;

		inline static constexpr char className[] = "Capsule";
		static Luna<Capsule_BindLua>::FunctionType methods[];
		static Luna<Capsule_BindLua>::PropertyType properties[];

		Capsule_BindLua(const Protean::Capsule& capsule) : capsule(capsule) {}
		Capsule_BindLua(lua_State* L);

		int Intersects(lua_State* L);
		int GetAABB(lua_State* L);
		int GetBase(lua_State* L);
		int GetTip(lua_State* L);
		int GetRadius(lua_State* L);
		int SetBase(lua_State* L);
		int SetTip(lua_State* L);
		int SetRadius(lua_State* L);
		int GetPlacementOrientation(lua_State* L);
	};
}

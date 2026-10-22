#include "script/bindings/physics_bind_lua.h"
#include "physics/physics.h"
#include "script/bindings/scene_bind_lua.h"
#include "script/bindings/math_bind_lua.h"
#include "script/bindings/primitive_bind_lua.h"

namespace Protean::Lua
{
	Luna<Physics_BindLua>::FunctionType Physics_BindLua::methods[] = {
		lunamethod(Physics_BindLua, SetEnabled),
		lunamethod(Physics_BindLua, IsEnabled),
		lunamethod(Physics_BindLua, SetSimulationEnabled),
		lunamethod(Physics_BindLua, IsSimulationEnabled),
		lunamethod(Physics_BindLua, SetInterpolationEnabled),
		lunamethod(Physics_BindLua, IsInterpolationEnabled),
		lunamethod(Physics_BindLua, SetDebugDrawEnabled),
		lunamethod(Physics_BindLua, IsDebugDrawEnabled),
		lunamethod(Physics_BindLua, SetAccuracy),
		lunamethod(Physics_BindLua, GetAccuracy),
		lunamethod(Physics_BindLua, SetFrameRate),
		lunamethod(Physics_BindLua, GetFrameRate),
		lunamethod(Physics_BindLua, SetPosition),
		lunamethod(Physics_BindLua, SetPositionAndRotation),
		lunamethod(Physics_BindLua, SetLinearVelocity),
		lunamethod(Physics_BindLua, SetAngularVelocity),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyForce),
		lunamethod(Physics_BindLua, ApplyForceAt),
		lunamethod(Physics_BindLua, ApplyImpulse),
		lunamethod(Physics_BindLua, ApplyImpulseAt),
		lunamethod(Physics_BindLua, ApplyTorque),
		lunamethod(Physics_BindLua, SetActivationState),
		lunamethod(Physics_BindLua, ActivateAllRigidBodies),
		lunamethod(Physics_BindLua, OptimizeBroadPhase),
		lunamethod(Physics_BindLua, ResetPhysicsObjects),
		lunamethod(Physics_BindLua, GetVelocity),
		lunamethod(Physics_BindLua, GetPosition),
		lunamethod(Physics_BindLua, GetRotation),
		lunamethod(Physics_BindLua, GetCharacterGroundPosition),
		lunamethod(Physics_BindLua, GetCharacterGroundNormal),
		lunamethod(Physics_BindLua, GetCharacterGroundVelocity),
		lunamethod(Physics_BindLua, IsCharacterGroundSupported),
		lunamethod(Physics_BindLua, GetCharacterGroundState),
		lunamethod(Physics_BindLua, ChangeCharacterShape),
		lunamethod(Physics_BindLua, MoveCharacter),
		lunamethod(Physics_BindLua, SetGhostMode),
		lunamethod(Physics_BindLua, SetRagdollGhostMode),
		lunamethod(Physics_BindLua, Intersects),
		lunamethod(Physics_BindLua, PickDrag),
		lunamethod(Physics_BindLua, DriveVehicle),
		lunamethod(Physics_BindLua, GetVehicleForwardVelocity),
		lunamethod(Physics_BindLua, SetCharacterCollisionTolerance),
		lunamethod(Physics_BindLua, GetCharacterCollisionTolerance),
		{ NULL, NULL }
	};
	Luna<Physics_BindLua>::PropertyType Physics_BindLua::properties[] = {
		{ NULL, NULL }
	};


	int Physics_BindLua::SetEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsEnabled(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Physics::IsEnabled());
		return 1;
	}
	int Physics_BindLua::SetSimulationEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetSimulationEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetSimulationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsSimulationEnabled(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Physics::IsSimulationEnabled());
		return 1;
	}
	int Physics_BindLua::SetInterpolationEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetInterpolationEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetInterpolationEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsInterpolationEnabled(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Physics::IsInterpolationEnabled());
		return 1;
	}
	int Physics_BindLua::SetDebugDrawEnabled(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetDebugDrawEnabled(Protean::Lua::SGetBool(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetDebugDrawEnabled(bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsDebugDrawEnabled(lua_State* L)
	{
		Protean::Lua::SSetBool(L, Protean::Physics::IsDebugDrawEnabled());
		return 1;
	}
	int Physics_BindLua::SetAccuracy(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetAccuracy(Protean::Lua::SGetInt(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetAccuracy(int value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetAccuracy(lua_State* L)
	{
		Protean::Lua::SSetInt(L, Protean::Physics::GetAccuracy());
		return 1;
	}
	int Physics_BindLua::SetFrameRate(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetFrameRate(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetFrameRate(float value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetFrameRate(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, Protean::Physics::GetFrameRate());
		return 1;
	}

	int Physics_BindLua::SetPosition(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::SetPosition(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "SetPosition(RigidBodyPhysicsComponent component, Vector position) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetPositionAndRotation(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 2)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec2 == nullptr)
			{
				Protean::Lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) third argument is not a Vector!");
				return 0;
			}
			Protean::Physics::SetPositionAndRotation(
				*component->component,
				vec->GetFloat3(),
				vec2->data
			);
		}
		else
			Protean::Lua::SError(L, "SetPositionAndRotation(RigidBodyPhysicsComponent component, Vector position, Vector rotationQuaternion) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetLinearVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::SetLinearVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "SetLinearVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetAngularVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::SetAngularVelocity(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "SetAngularVelocity(RigidBodyPhysicsComponent component, Vector velocity) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForce(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::ApplyForce(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "ApplyForce(RigidBodyPhysicsComponent component, Vector force) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyForceAt(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 2)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = Protean::Lua::SGetBool(L, 4);

			Protean::Physics::ApplyForceAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			Protean::Lua::SError(L, "ApplyForceAt(RigidBodyPhysicsComponent component, Vector force, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulse(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				SceneBindings::HumanoidComponent_BindLua* humanoid = Luna<SceneBindings::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					Protean::Lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) first argument is not a RigidBodyPhysicsComponent!");
					Protean::Lua::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) first argument is not a HumanoidComponent!");
					return 0;
				}
				Protean::HumanoidComponent::HumanoidBone bone = (Protean::HumanoidComponent::HumanoidBone)Protean::Lua::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					Protean::Lua::SError(L, "ApplyImpulse(HumanoidComponent component, HumanoidBone bone, Vector impulse) third argument is not a Vector!");
					return 0;
				}
				Protean::Physics::ApplyImpulse(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::ApplyImpulse(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "ApplyImpulse(RigidBodyPhysicsComponent component, Vector impulse) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyImpulseAt(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 2)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				SceneBindings::HumanoidComponent_BindLua* humanoid = Luna<SceneBindings::HumanoidComponent_BindLua>::lightcheck(L, 1);
				if (humanoid == nullptr)
				{
					Protean::Lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) first argument is not a RigidBodyPhysicsComponent!");
					Protean::Lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) first argument is not a HumanoidComponent!");
					return 0;
				}
				Protean::HumanoidComponent::HumanoidBone bone = (Protean::HumanoidComponent::HumanoidBone)Protean::Lua::SGetInt(L, 2);
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 3);
				if (vec == nullptr)
				{
					Protean::Lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) third argument is not a Vector!");
					return 0;
				}
				Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 4);
				if (vec2 == nullptr)
				{
					Protean::Lua::SError(L, "ApplyImpulseAt(HumanoidComponent component, HumanoidBone bone, Vector impulse, Vector at) fourth argument is not a Vector!");
					return 0;
				}
				bool at_local = true;
				if (argc > 4)
					at_local = Protean::Lua::SGetBool(L, 5);

				Protean::Physics::ApplyImpulseAt(
					*humanoid->component,
					bone,
					*(XMFLOAT3*)vec,
					*(XMFLOAT3*)vec2,
					at_local
				);
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) second argument is not a Vector!");
				return 0;
			}
			Vector_BindLua* vec2 = Luna<Vector_BindLua>::lightcheck(L, 3);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) third argument is not a Vector!");
				return 0;
			}
			bool at_local = true;
			if (argc > 3)
				at_local = Protean::Lua::SGetBool(L, 4);

			Protean::Physics::ApplyImpulseAt(
				*component->component,
				*(XMFLOAT3*)vec,
				*(XMFLOAT3*)vec2,
				at_local
			);
		}
		else
			Protean::Lua::SError(L, "ApplyImpulseAt(RigidBodyPhysicsComponent component, Vector impulse, Vector at) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ApplyTorque(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) second argument is not a Vector!");
				return 0;
			}
			Protean::Physics::ApplyTorque(
				*component->component,
				*(XMFLOAT3*)vec
			);
		}
		else
			Protean::Lua::SError(L, "ApplyTorque(RigidBodyPhysicsComponent component, Vector torque) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetActivationState(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* rigid = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rigid == nullptr)
			{
				SceneBindings::SoftBodyPhysicsComponent_BindLua* soft = Luna<SceneBindings::SoftBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
				if (soft == nullptr)
				{
					Protean::Lua::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) first argument is not a RigidBodyPhysicsComponent or SoftBodyPhysicsComponent!");
					return 0;
				}
				Protean::Physics::SetActivationState(
					*soft->component,
					(Protean::Physics::ActivationState)Protean::Lua::SGetInt(L, 2)
				);
				return 0;
			}
			Protean::Physics::SetActivationState(
				*rigid->component,
				(Protean::Physics::ActivationState)Protean::Lua::SGetInt(L, 2)
			);
		}
		else
			Protean::Lua::SError(L, "SetActivationState(RigidBodyPhysicsComponent | SoftBodyPhysicsComponent component, int state) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ActivateAllRigidBodies(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::Scene_BindLua* scene = Luna<SceneBindings::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Protean::Lua::SError(L, "ActivateAllRigidBodies(Scene scene) first argument is not a Scene!");
				return 0;
			}
			Protean::Physics::ActivateAllRigidBodies(*scene->scene);
		}
		else
			Protean::Lua::SError(L, "ActivateAllRigidBodies(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::OptimizeBroadPhase(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::Scene_BindLua* scene = Luna<SceneBindings::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Protean::Lua::SError(L, "OptimizeBroadPhase(Scene scene) first argument is not a Scene!");
				return 0;
			}
			Protean::Physics::OptimizeBroadPhase(*scene->scene);
		}
		else
			Protean::Lua::SError(L, "OptimizeBroadPhase(Scene scene, [opt bool force]) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ResetPhysicsObjects(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::Scene_BindLua* scene = Luna<SceneBindings::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Protean::Lua::SError(L, "ResetPhysicsObjects(Scene scene) first argument is not a Scene!");
				return 0;
			}
			Protean::Physics::ResetPhysicsObjects(*scene->scene);
		}
		else
			Protean::Lua::SError(L, "ResetPhysicsObjects(Scene scene) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetVelocity(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetPosition(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetPosition(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetRotation(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetRotation(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetRotation(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetRotation(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundPosition(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetCharacterGroundPosition(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetCharacterGroundPosition(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundNormal(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetCharacterGroundNormal(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetCharacterGroundNormal(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Luna<Vector_BindLua>::push(L, Protean::Physics::GetCharacterGroundVelocity(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetCharacterGroundVelocity(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::IsCharacterGroundSupported(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Protean::Lua::SSetBool(L, Protean::Physics::IsCharacterGroundSupported(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "IsCharacterGroundSupported(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetCharacterGroundState(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Protean::Lua::SSetInt(L, (int)Protean::Physics::GetCharacterGroundState(*component->component));
			return 1;
		}
		else
			Protean::Lua::SError(L, "GetCharacterGroundState(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::ChangeCharacterShape(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Protean::RigidBodyPhysicsComponent::CapsuleParams capsule;
			if (argc > 1)
			{
				capsule.height = Protean::Lua::SGetFloat(L, 2);
				if (argc > 2)
				{
					capsule.radius = Protean::Lua::SGetFloat(L, 3);
				}
			}
			Protean::Lua::SSetBool(L, Protean::Physics::ChangeCharacterShape(*component->component, capsule));
			return 1;
		}
		else
			Protean::Lua::SError(L, "ChangeCharacterShape(RigidBodyPhysicsComponent component) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::MoveCharacter(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}

			Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, 2);
			if (vec == nullptr)
			{
				Protean::Lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) second argument is not a Vector!");
				return 0;
			}

			float movement_speed = 6.0f;
			float jump = 0;
			bool controlMovementDuringJump = false;

			if (argc > 2)
			{
				movement_speed = Protean::Lua::SGetFloat(L, 3);
				if (argc > 3)
				{
					jump = Protean::Lua::SGetFloat(L, 4);
					if (argc > 4)
					{
						controlMovementDuringJump = Protean::Lua::SGetBool(L, 5);
					}
				}
			}

			Protean::Physics::MoveCharacter(
				*component->component,
				vec->GetFloat3(),
				movement_speed,
				jump,
				controlMovementDuringJump
			);
		}
		else
			Protean::Lua::SError(L, "MoveCharacter(RigidBodyPhysicsComponent component, Vector movement_direction, opt float movement_speed = 6, opt float jump = 0, opt bool controlMovementDuringJump = false) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetGhostMode(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* component = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (component != nullptr)
			{
				bool value = Protean::Lua::SGetBool(L, 2);
				Protean::Physics::SetGhostMode(*component->component, value);
				return 0;
			}
			SceneBindings::HumanoidComponent_BindLua* humanoidcomponent = Luna<SceneBindings::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (humanoidcomponent != nullptr)
			{
				bool value = Protean::Lua::SGetBool(L, 2);
				Protean::Physics::SetGhostMode(*humanoidcomponent->component, value);
				return 0;
			}
		}
		else
			Protean::Lua::SError(L, "SetGhostMode(RigidBodyPhysicsComponent|HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::SetRagdollGhostMode(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::HumanoidComponent_BindLua* component = Luna<SceneBindings::HumanoidComponent_BindLua>::lightcheck(L, 1);
			if (component == nullptr)
			{
				Protean::Lua::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			bool value = Protean::Lua::SGetBool(L, 2);

			Protean::Physics::SetRagdollGhostMode(*component->component, value);
			return 0;
		}
		else
			Protean::Lua::SError(L, "SetRagdollGhostMode(HumanoidComponent component, bool value) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::SetCharacterCollisionTolerance(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			Protean::Physics::SetCharacterCollisionTolerance(Protean::Lua::SGetFloat(L, 1));
		}
		else
			Protean::Lua::SError(L, "SetCharacterCollisionTolerance(float value) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::GetCharacterCollisionTolerance(lua_State* L)
	{
		Protean::Lua::SSetFloat(L, Protean::Physics::GetCharacterCollisionTolerance());
		return 1;
	}

	int Physics_BindLua::Intersects(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 1)
		{
			SceneBindings::Scene_BindLua* scene = Luna<SceneBindings::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Protean::Lua::SError(L, "Intersects(Scene, Ray) first argument is not a Scene!");
				return 0;
			}
			PrimitiveBindings::Ray_BindLua* ray = Luna<PrimitiveBindings::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				Protean::Lua::SError(L, "Intersects(Scene, Ray) second argument is not a Ray!");
				return 0;
			}

			Protean::Physics::RayIntersectionResult result = Protean::Physics::Intersects(*scene->scene, ray->ray);
			Protean::Lua::SSetLongLong(L, result.entity);
			Luna<Vector_BindLua>::push(L, result.position);
			Luna<Vector_BindLua>::push(L, result.normal);
			Protean::Lua::SSetLongLong(L, result.humanoid_ragdoll_entity);
			Protean::Lua::SSetInt(L, (int)result.humanoid_bone);
			Luna<Vector_BindLua>::push(L, result.position_local);
			return 6;
		}
		Protean::Lua::SError(L, "Intersects(Scene, Ray) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::PickDrag(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 2)
		{
			SceneBindings::Scene_BindLua* scene = Luna<SceneBindings::Scene_BindLua>::lightcheck(L, 1);
			if (scene == nullptr)
			{
				Protean::Lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) first argument is not a Scene!");
				return 0;
			}
			PrimitiveBindings::Ray_BindLua* ray = Luna<PrimitiveBindings::Ray_BindLua>::lightcheck(L, 2);
			if (ray == nullptr)
			{
				Protean::Lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) second argument is not a Ray!");
				return 0;
			}
			PickDragOperation_BindLua* op = Luna<PickDragOperation_BindLua>::lightcheck(L, 3);
			if (op == nullptr)
			{
				Protean::Lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) third argument is not a PickDragOperation!");
				return 0;
			}

			Protean::Physics::ConstraintType type = Protean::Physics::ConstraintType_Fixed;
			float break_distance = FLT_MAX;

			if (argc > 3)
			{
				type = (Protean::Physics::ConstraintType)Protean::Lua::SGetInt(L, 4);

				if (argc > 4)
				{
					break_distance = Protean::Lua::SGetFloat(L, 5);
				}
			}

			Protean::Physics::PickDrag(*scene->scene, ray->ray, op->op, type, break_distance);
			return 0;
		}
		Protean::Lua::SError(L, "PickDrag(Scene, Ray, PickDragOperation) not enough arguments!");
		return 0;
	}

	int Physics_BindLua::DriveVehicle(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* rb = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				Protean::Lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				Protean::Lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			float forward = 0;
			float right = 0;
			float brake = 0;
			float handbrake = 0;

			if (argc > 1)
			{
				forward = Protean::Lua::SGetFloat(L, 2);

				if (argc > 2)
				{
					right = Protean::Lua::SGetFloat(L, 3);

					if (argc > 3)
					{
						brake = Protean::Lua::SGetFloat(L, 4);

						if (argc > 4)
						{
							handbrake = Protean::Lua::SGetFloat(L, 5);
						}
					}
				}
			}

			Protean::Physics::DriveVehicle(*rb->component, forward, right, brake, handbrake);
			return 0;
		}
		Protean::Lua::SError(L, "DriveVehicle(RigidBodyPhysicsComponent rigidbody, opt float forward = 0, opt float right = 0, opt float brake = 0, opt float handbrake = 0) not enough arguments!");
		return 0;
	}
	int Physics_BindLua::GetVehicleForwardVelocity(lua_State* L)
	{
		int argc = Protean::Lua::SGetArgCount(L);
		if (argc > 0)
		{
			SceneBindings::RigidBodyPhysicsComponent_BindLua* rb = Luna<SceneBindings::RigidBodyPhysicsComponent_BindLua>::lightcheck(L, 1);
			if (rb == nullptr)
			{
				Protean::Lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) first argument is not a RigidBodyPhysicsComponent!");
				return 0;
			}
			if (!rb->component->IsVehicle())
			{
				Protean::Lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) argument is a RigidBodyPhysicsComponent, but not a vehicle!");
				return 0;
			}

			Protean::Lua::SSetFloat(L, Protean::Physics::GetVehicleForwardVelocity(*rb->component));
			return 1;
		}
		Protean::Lua::SError(L, "GetVehicleForwardVelocity(RigidBodyPhysicsComponent rigidbody) not enough arguments!");
		return 0;
	}

	void Physics_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<PickDragOperation_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<Physics_BindLua>::Register(Protean::Lua::GetLuaState());
			Luna<Physics_BindLua>::push_global(Protean::Lua::GetLuaState(), "physics");

			Protean::Lua::RunText(R"(
ACTIVATION_STATE_ACTIVE = 0
ACTIVATION_STATE_INACTIVE = 1

ConstraintType = {
	Fixed = 0,
	Point = 1
}

CharacterGroundStates = {
	OnGround = 0,
	OnSteepGround = 1,
	NotSupported = 2,
	InAir = 3,
}
)");
		}
	}




	Luna<PickDragOperation_BindLua>::FunctionType PickDragOperation_BindLua::methods[] = {
		lunamethod(PickDragOperation_BindLua, Finish),
		{ NULL, NULL }
	};
	Luna<PickDragOperation_BindLua>::PropertyType PickDragOperation_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int PickDragOperation_BindLua::Finish(lua_State* L)
	{
		op = {};
		return 0;
	}
}

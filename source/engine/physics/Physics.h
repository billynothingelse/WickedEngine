#pragma once
#include "core/ECS.h"
#include "scene/Scene.h"
#include "core/JobSystem.h"
#include "math/Primitive.h"

#include <memory>

namespace Protean::Physics
{
	// Initializes the physics engine
	void Initialize();

	// Enable/disable the physics engine all together
	void SetEnabled(bool value);
	bool IsEnabled();

	// Enable/disable the physics simulation.
	//	Physics engine state will be updated but not simulated
	void SetSimulationEnabled(bool value);
	bool IsSimulationEnabled();

	// Enable/disable the physics interpolation.
	//	When enabled, physics simulation's fixed frame rate will be
	//	interpolated to match the variable frame rate of rendering
	void SetInterpolationEnabled(bool value);
	bool IsInterpolationEnabled();

	// Enable/disable debug drawing of physics objects
	void SetDebugDrawEnabled(bool value);
	bool IsDebugDrawEnabled();

	// Adjust constraint debugging sizes
	void SetConstraintDebugSize(float value);
	float GetConstraintDebugSize();

	// Set maximum distance for debug drawing physics shapes
	void SetDebugDrawMaxDistance(float value);
	float GetDebugDrawMaxDistance();

	// Set the accuracy of the simulation
	//	This value corresponds to maximum simulation step count
	//	Higher values will be slower but more accurate
	void SetAccuracy(int value);
	int GetAccuracy();

	// Set frames per second value of physics simulation (default = 120 FPS)
	void SetFrameRate(float value);
	float GetFrameRate();

	// Set character collision tolerance
	//	Higher values push the character farther from surfaces,
	//	preventing it from falling through large triangles or complex geometry
	void SetCharacterCollisionTolerance(float value);
	float GetCharacterCollisionTolerance();

	// Update the physics state, run simulation, etc.
	void RunPhysicsUpdateSystem(
		Protean::JobSystem::context& ctx,
		Protean::Scene& scene,
		float dt
	);

	// Teleport a dynamic rigid body:
	void SetPosition(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position
	);
	void SetPositionAndRotation(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& position,
		const XMFLOAT4& rotation
	);

	// Set linear velocity to rigid body
	void SetLinearVelocity(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	);
	// Set angular velocity to rigid body
	void SetAngularVelocity(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& velocity
	);

	XMFLOAT3 GetVelocity(Protean::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetPosition(Protean::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT4 GetRotation(Protean::RigidBodyPhysicsComponent& physicscomponent);

	XMFLOAT3 GetCharacterGroundPosition(Protean::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetCharacterGroundNormal(Protean::RigidBodyPhysicsComponent& physicscomponent);
	XMFLOAT3 GetCharacterGroundVelocity(Protean::RigidBodyPhysicsComponent& physicscomponent);

	// Returns true if character is standing on the ground or steep ground, false otherwise
	bool IsCharacterGroundSupported(Protean::RigidBodyPhysicsComponent& physicscomponent);

	enum CharacterGroundStates {
		CharacterGroundStates_OnGround,		// Character is on the ground and can move freely.
		CharacterGroundStates_OnSteepGround,	// Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
		CharacterGroundStates_NotSupported,	// Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
		CharacterGroundStates_InAir,			// Character is in the air and is not touching anything.
	};
	CharacterGroundStates GetCharacterGroundState(Protean::RigidBodyPhysicsComponent& physicscomponent);

	// Tries to change the physics character's shape, returns tru if successful, false if character would be blocked with the new shape
	bool ChangeCharacterShape(Protean::RigidBodyPhysicsComponent& physicscomponent, const Protean::RigidBodyPhysicsComponent::CapsuleParams& capsule);

	// Apply movement logic to a physics character
	//	movement_direction : a normalized or zero direction
	//	movement_speed	: horizontal movement speed of the character
	//	jump	: vertical jump speed (positive)
	//	controlMovementDuringJump	: whether movement can be applied when character is in the air
	void MoveCharacter(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& movement_direction,
		float movement_speed = 6,
		float jump = 0,
		bool controlMovementDuringJump = false
	);

	// Apply force at body center
	void ApplyForce(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force
	);
	// Apply force at position
	void ApplyForceAt(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& force,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);

	// Apply impulse at body center
	void ApplyImpulse(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse
	);
	void ApplyImpulse(
		Protean::HumanoidComponent& humanoid,
		Protean::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse
	);
	// Apply impulse at position
	void ApplyImpulseAt(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);
	void ApplyImpulseAt(
		Protean::HumanoidComponent& humanoid,
		Protean::HumanoidComponent::HumanoidBone bone,
		const XMFLOAT3& impulse,
		const XMFLOAT3& at,
		bool at_local = true // whether at is in local space of the body or not (global)
	);

	void ApplyTorque(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& torque
	);

	// Set input from driver
	//	forward:	Value between -1 and 1 for auto transmission and value between 0 and 1 indicating desired driving direction and amount the gas pedal is pressed
	//	right:		Value between -1 and 1 indicating desired steering angle (1 = right)
	//	brake:		Value between 0 and 1 indicating how strong the brake pedal is pressed
	//	handbrake:	Value between 0 and 1 indicating how strong the hand brake is pulled (back brake for motorcycles)
	void DriveVehicle(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		float forward,
		float right = 0,
		float brake = 0,
		float handbrake = 0
	);

	// Signed velocity amount in forward direction
	float GetVehicleForwardVelocity(Protean::RigidBodyPhysicsComponent& physicscomponent);

	// Override all vehicle wheel transforms in world space
	void OverrideWehicleWheelTransforms(Protean::Scene& scene);

	bool IsConstraintBroken(const Protean::PhysicsConstraintComponent& physicscomponent);
	void SetConstraintBroken(Protean::PhysicsConstraintComponent& physicscomponent, bool broken = true);

	enum ActivationState {
		ActivationState_Active,
		ActivationState_Inactive,
	};
	void SetActivationState(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		ActivationState state
	);
	void SetActivationState(
		Protean::SoftBodyPhysicsComponent& physicscomponent,
		ActivationState state
	);
	void ActivateAllRigidBodies(Protean::Scene& scene);

	// Request broadphase optimization to reclaim nodes from previously removed bodies
	void OptimizeBroadPhase(Protean::Scene& scene);

	void ResetPhysicsObjects(Protean::Scene& scene);

	XMFLOAT3 GetSoftBodyNodePosition(
		Protean::SoftBodyPhysicsComponent& physicscomponent,
		uint32_t physicsIndex
	);

	void SetGhostMode(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		bool value
	);
	void SetGhostMode(
		Protean::HumanoidComponent& humanoid,
		bool value
	);
	void SetRagdollGhostMode(Protean::HumanoidComponent& humanoid, bool value);

	struct RayIntersectionResult
	{
		Protean::Entity entity = Protean::INVALID_ENTITY;
		XMFLOAT3 position = XMFLOAT3(0, 0, 0);
		XMFLOAT3 position_local = XMFLOAT3(0, 0, 0);
		XMFLOAT3 normal = XMFLOAT3(0, 0, 0);
		Protean::Entity humanoid_ragdoll_entity = Protean::INVALID_ENTITY;
		Protean::HumanoidComponent::HumanoidBone humanoid_bone = Protean::HumanoidComponent::HumanoidBone_Count;
		int softbody_triangleID = -1;
		const void* physicsobject = nullptr;
		constexpr bool IsValid() const { return entity != Protean::INVALID_ENTITY; }
	};
	RayIntersectionResult Intersects(
		const Protean::Scene& scene,
		Protean::Ray ray
	);

	struct PickDragOperation
	{
		Protean::Allocator::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state != nullptr; }
	};
	enum ConstraintType {
		ConstraintType_Fixed,	// Fixes the whole object with the current orientation
		ConstraintType_Point,	// Fixes the object to rotate around a point
	};
	void PickDrag(
		const Protean::Scene& scene,
		Protean::Ray ray,
		PickDragOperation& op,
		ConstraintType constraint_type = ConstraintType_Fixed,
		float break_distance = FLT_MAX
	);

	// Create the shape immediately, useful when you want to do this from a thread
	//	Otherwise this will be created on demand before it's used for the first time
	void CreateRigidBodyShape(
		Protean::RigidBodyPhysicsComponent& physicscomponent,
		const XMFLOAT3& scale_local = XMFLOAT3(1, 1, 1),
		const Protean::MeshComponent* mesh = nullptr
	);
}

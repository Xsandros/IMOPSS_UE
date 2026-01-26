#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/Rig/DeliveryRigV3.h"

#include "DeliverySpecV3.generated.h"

// ============================================================
// Typed configs (keep these stable - used by driver implementations)
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryInstantQueryConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	float Range = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	int32 MaxHits = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	bool bMultiHit = false;
};

USTRUCT(BlueprintType)
struct FDeliveryFieldConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Field")
	float TickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Field")
	bool bEmitEnterExit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Field")
	bool bEmitStay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Field")
	bool bStayPerTick = true;
};

UENUM(BlueprintType)
enum class EMoverMotionKindV3 : uint8
{
	Straight,
	Ballistic,
	Homing
};

USTRUCT(BlueprintType)
struct FDeliveryMoverConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	EMoverMotionKindV3 Motion = EMoverMotionKindV3::Straight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float Speed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float MaxDistance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float GravityScale = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float HomingStrength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	FName HomingTargetSet = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bStopOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bPierce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	int32 MaxPierceHits = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float TickInterval = 0.f; // 0 = per-frame
};

USTRUCT(BlueprintType)
struct FDeliveryBeamConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Range = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float TickInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	bool bLockOnTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	FName LockTargetSet = NAME_None;
};

USTRUCT(BlueprintType)
struct FDeliveryDebugDrawConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bEnable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	float Duration = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawShape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawHits = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawRig = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawRigAxes = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	float RigPointSize = 12.f;
};

USTRUCT(BlueprintType)
struct FDeliveryEventHitTagsV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Started;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Stopped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Hit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Enter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Stay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Exit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags")
	FGameplayTagContainer Tick;
};

// ============================================================
// Composite primitives: anchor refs + motion stack (data only)
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryAnchorRefKindV3 : uint8
{
	Root,
	EmitterIndex,
	// Future-proofing hooks (we won't implement resolver logic yet in Etappe 1)
	EmitterName,
	CasterSocket,
	TargetSet
};

USTRUCT(BlueprintType)
struct FDeliveryAnchorRefV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	EDeliveryAnchorRefKindV3 Kind = EDeliveryAnchorRefKindV3::Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	int32 EmitterIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FName EmitterName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FName TargetSetName = NAME_None;

	// Applied after anchor pose is resolved
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalScale = FVector(1,1,1);
};

UENUM(BlueprintType)
enum class EDeliveryMotionOpKindV3 : uint8
{
	None,
	OrbitAroundAxis,   // orbit around axis from Blackboard or derived (future)
	HelixAroundAxis,   // orbit + forward
	SplineFollow,      // follow spline (future)
	LagSpring,         // follow with lag
	SeekTarget,        // homing/seek (future)
	Noise              // deterministic noise
};

USTRUCT(BlueprintType)
struct FDeliveryMotionOpV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	EDeliveryMotionOpKindV3 Kind = EDeliveryMotionOpKindV3::None;

	// Generic params (kept flexible, deterministic)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	float A = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	float B = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	float C = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	FVector V = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	int32 I = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	bool b = false;
};

USTRUCT(BlueprintType)
struct FDeliveryMotionStackV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Motion")
	TArray<FDeliveryMotionOpV3> Ops;
};

// ============================================================
// Blackboard init + ownership rules (data only in spec; enforcement in runtime)
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryBBValueTypeV3 : uint8
{
	Bool,
	Int,
	Float,
	Vector,
	Rotator,
	Transform
};

UENUM(BlueprintType)
enum class EDeliveryBBOwnerV3 : uint8
{
	Group,
	Rig,
	Target,
	DerivedMotion,
	PrimitiveMotion,
	Driver,
	Stop
};

UENUM(BlueprintType)
enum class EDeliveryBBPhaseV3 : uint8
{
	Time,
	Rig,
	Target,
	DerivedMotion,
	PrimitiveMotion,
	Query,
	Stop,
	Commit
};

USTRUCT(BlueprintType)
struct FDeliveryBBKeySpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	EDeliveryBBValueTypeV3 Type = EDeliveryBBValueTypeV3::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	EDeliveryBBOwnerV3 Owner = EDeliveryBBOwnerV3::Group;

	// Bitmask: which phases allow writing.
	// We store as uint8 bitmask for simplicity: (1<<Phase)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	uint8 WritePhaseMask = 0xFF; // default: allow all, will tighten via defaults in runtime

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FName Unit = NAME_None; // optional (e.g. "cm", "sec")
};

USTRUCT(BlueprintType)
struct FDeliveryBBInitValueV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	EDeliveryBBValueTypeV3 Type = EDeliveryBBValueTypeV3::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	bool BoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	int32 IntValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	float FloatValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FVector VectorValue = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FRotator RotatorValue = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	FTransform TransformValue = FTransform::Identity;
};

USTRUCT(BlueprintType)
struct FDeliveryBlackboardInitSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	TArray<FDeliveryBBKeySpecV3> Keys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Blackboard")
	TArray<FDeliveryBBInitValueV3> InitialValues;
};

USTRUCT(BlueprintType)
struct FDeliveryOwnershipRulesV3
{
	GENERATED_BODY()

	// If true, invalid writes will ensure/log (recommended for dev)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ownership")
	bool bStrict = true;

	// If true, invalid writes are ignored instead of overwriting (safer for MP determinism)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ownership")
	bool bIgnoreInvalidWrites = true;
};

// ============================================================
// Primitive Spec (heterogeneous), Group Spec (composite-first)
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryPrimitiveSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName PrimitiveId = "P0";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	EDeliveryKindV3 Kind = EDeliveryKindV3::InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryQueryPolicyV3 Query;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryStopPolicyV3 StopPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryAnchorRefV3 Anchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryMotionStackV3 Motion;

	// Delivery/Hit tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FGameplayTagContainer DeliveryTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FGameplayTagContainer HitTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryEventHitTagsV3 EventHitTags;

	// Optional: write hits into a target set name (resolved in runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName OutTargetSetName = NAME_None;

	// Presentation hooks (data only; no rendering required yet)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FGameplayTagContainer PresentationTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryDebugDrawConfigV3 DebugDrawOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	bool bOverrideDebugDraw = false;

	// Typed configs
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryInstantQueryConfigV3 InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryMoverConfigV3 Mover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryBeamConfigV3 Beam;
};

USTRUCT(BlueprintType)
struct FDeliverySpecV3
{
	GENERATED_BODY()

	// Stable id for this delivery group (used with handle + instance index)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName DeliveryId = NAME_None;

	// Where this group attaches (root space)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryAttachV3 Attach;

	// Shared rig skeleton/animator
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryRigV3 Rig;

	// Pose evaluation policy (shared)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	EDeliveryPoseUpdatePolicyV3 PoseUpdatePolicy = EDeliveryPoseUpdatePolicyV3::EveryTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	float PoseUpdateInterval = 0.f; // used when PoseUpdatePolicy == Interval

	// Group defaults
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FGameplayTagContainer DeliveryTagsDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FGameplayTagContainer HitTagsDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryEventHitTagsV3 EventHitTagsDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName OutTargetSetDefault = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryDebugDrawConfigV3 DebugDrawDefaults;

	// Composite primitives (required; Single = one entry)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	TArray<FDeliveryPrimitiveSpecV3> Primitives;

	// Blackboard + ownership (future-proof; enforced by runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryBlackboardInitSpecV3 BlackboardInit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryOwnershipRulesV3 OwnershipRules;
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "Delivery/DeliveryTypesV3.h"

#include "DeliverySpecV3.generated.h"

// ============================================================
// Debug draw config
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryDebugDrawConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bEnable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawHits = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawShape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	float Duration = 0.05f;
};

// ============================================================
// Anchor reference
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryAnchorRefKindV3 : uint8
{
	Root,
	EmitterIndex,
	EmitterName,  // reserved
	CasterSocket, // reserved
	TargetSet     // reserved
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

	// Local offsets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalScale = FVector(1.f, 1.f, 1.f);
};

// ============================================================
// Per-kind configs
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryInstantQueryConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	float Range = 1000.f;

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
	float TickInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Field")
	bool bEmitEnterExit = false;
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
	float TickInterval = 0.f; // 0 = every tick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	EMoverMotionKindV3 Motion = EMoverMotionKindV3::Straight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float Speed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float MaxDistance = 0.f; // 0 = unlimited

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bStopOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bPierce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	int32 MaxPierceHits = 0; // 0 = unlimited (if bPierce)

	// Ballistic
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float GravityScale = 1.f;

	// Homing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	FName HomingTargetSet = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float HomingStrength = 5.f;
};

USTRUCT(BlueprintType)
struct FDeliveryBeamConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float TickInterval = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Range = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Radius = 0.f; // 0 = line trace, >0 = sphere sweep

	// Optional lock-on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	bool bLockOnTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	FName LockTargetSet = NAME_None;
};

// ============================================================
// Blackboard (Phase/Owner enforcement scaffolding)
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryBBValueTypeV3 : uint8
{
	Float,
	Int,
	Bool,
	Vector,
	Rotator,
	Transform,
	Name
};

UENUM(BlueprintType)
enum class EDeliveryBBOwnerV3 : uint8
{
	Group,
	Primitive,
	System
};

UENUM(BlueprintType)
enum class EDeliveryBBPhaseV3 : uint8
{
	Time,
	Rig,
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	EDeliveryBBValueTypeV3 Type = EDeliveryBBValueTypeV3::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	EDeliveryBBOwnerV3 Owner = EDeliveryBBOwnerV3::Group;

	// Bitmask of allowed write phases: (1<<Phase)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	uint8 WritePhaseMask = 0xFF;
};

USTRUCT(BlueprintType)
struct FDeliveryBlackboardInitSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	TArray<FDeliveryBBKeySpecV3> Keys;
};

USTRUCT(BlueprintType)
struct FDeliveryOwnershipRulesV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	bool bEnforcePhase = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	bool bEnforceOwner = true;

	// If true, unknown keys are rejected (recommended for determinism).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	bool bRejectUnknownKeys = true;
};

// ============================================================
// Primitive spec
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryPrimitiveSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	EDeliveryKindV3 Kind = EDeliveryKindV3::InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryAnchorRefV3 Anchor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryQueryPolicyV3 Query;

	// Optional output (write hits/members into TargetStore)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName OutTargetSetName = NAME_None;

	// Kind configs (use only what matches Kind)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryInstantQueryConfigV3 InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryMoverConfigV3 Mover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryBeamConfigV3 Beam;

	// Debug
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	bool bOverrideDebugDraw = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryDebugDrawConfigV3 DebugDrawOverride;

	// Tags reserved for later (Presentation/Effects)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FGameplayTagContainer DeliveryTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FGameplayTagContainer HitTags;
};

// ============================================================
// Group spec
// ============================================================

USTRUCT(BlueprintType)
struct FDeliverySpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName DeliveryId = NAME_None;

	// Root attachment and rig
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryAttachV3 Attach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	TObjectPtr<class UDeliveryRigV3> Rig = nullptr;

	// Pose update policy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	EDeliveryPoseUpdatePolicyV3 PoseUpdatePolicy = EDeliveryPoseUpdatePolicyV3::EveryTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	float PoseUpdateInterval = 0.f;

	// Defaults
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryDebugDrawConfigV3 DebugDrawDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName OutTargetSetDefault = NAME_None;

	// Blackboard
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryBlackboardInitSpecV3 BlackboardInit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryOwnershipRulesV3 OwnershipRules;

	// Primitives
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	TArray<FDeliveryPrimitiveSpecV3> Primitives;
};

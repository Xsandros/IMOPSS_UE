#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Delivery/Motion/DeliveryMotionTypesV3.h"
#include "Engine/EngineTypes.h" // FCollisionProfileName, ECollisionChannel, FCollisionQueryParams

#include "Delivery/DeliveryTypesV3.h"

#include "DeliverySpecV3.generated.h"

class UDeliveryRigV3;

// ============================================================
// Debug draw
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryDebugDrawConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bEnable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	float Duration = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawShape = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawHits = true;
};

// ============================================================
// Anchor ref (primitive -> rig output)
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryAnchorRefKindV3 : uint8
{
	Root,
	EmitterIndex,
	PrimitiveId, // NEW: anchor to another primitive in the same group
	World          // NEW: absolute/world anchor
};

UENUM(BlueprintType)
enum class EDeliveryMissingAnchorPolicyV3 : uint8
{
	Freeze,         // keep last AnchorPoseWS/FinalPoseWS
	FallbackToRoot, // use group root pose
	StopSelf,       // stop this primitive (model 1 friendly)
};

USTRUCT(BlueprintType)
struct FDeliveryAnchorRefV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	EDeliveryAnchorRefKindV3 Kind = EDeliveryAnchorRefKindV3::Root;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor",
			  meta=(EditCondition="Kind==EDeliveryAnchorRefKindV3::EmitterIndex", EditConditionHides))
	int32 EmitterIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor",
			  meta=(EditCondition="Kind==EDeliveryAnchorRefKindV3::PrimitiveId", EditConditionHides))
	FName TargetPrimitiveId = NAME_None;

	// FollowMode bleibt immer sichtbar (Freeze default hast du schon)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	EDeliveryAnchorFollowModeV3 FollowMode = EDeliveryAnchorFollowModeV3::FreezeOnPlace;

	// Local transform bleibt sichtbar, auch für World (dann ist es world-transform / world-offset)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FRotator LocalRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Anchor")
	FVector LocalScale = FVector(1.f,1.f,1.f);


};

USTRUCT(BlueprintType)
struct FDeliveryQuerySpecV3
{
	GENERATED_BODY()

	// What kind of world query to perform (Overlap / Sweep / LineTrace)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryModeV3 Mode = EDeliveryQueryModeV3::Sweep;

	// How to select collision filtering (Channel / ObjectTypes / Profile)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryFilterModeV3 FilterMode = EDeliveryQueryFilterModeV3::ByChannel;

	// Used when FilterMode == ByChannel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query", meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByChannel"))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Used when FilterMode == ByObjectType
	// NOTE: We store as collision channels because UE object types map to collision channels.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query", meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByObjectType"))
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypes;

	// Used when FilterMode == ByProfile
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query", meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByProfile"))
	FCollisionProfileName CollisionProfile;

	// Common flags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	bool bIgnoreCaster = true;
};


// ============================================================
// Blackboard ownership (Write & Owner Matrix)
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
	Time = 0,
	Rig = 1,
	DerivedMotion = 2,
	PrimitiveMotion = 3,
	Query = 4,
	Stop = 5,
	Commit = 6
};

USTRUCT(BlueprintType)
struct FDeliveryBBKeySpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	FName Key = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	EDeliveryBBValueTypeV3 Type = EDeliveryBBValueTypeV3::Float;

	// Who is allowed to write
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	EDeliveryBBOwnerV3 Owner = EDeliveryBBOwnerV3::Group;

	// Bitmask of allowed phases for writing (bit = (1<<PhaseEnum))
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
	bool bEnforceOwner = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	bool bEnforcePhase = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|BB")
	bool bRejectUnknownKeys = true;
};

// ============================================================
// Per-kind configs
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryInstantQueryConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	float Range = 800.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	bool bMultiHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|InstantQuery")
	int32 MaxHits = 8;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover",
			  meta=(DisplayName="Trajectory Kind"))
	EMoverMotionKindV3 Motion = EMoverMotionKindV3::Straight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float TickInterval = 0.f; // 0 = every tick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float Speed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float GravityScale = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	float MaxDistance = 0.f; // 0 = infinite

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bStopOnHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bPierce = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	int32 MaxPierceHits = 0; // 0 = infinite

	// Homing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover",
			  meta=(EditCondition="Motion==EMoverMotionKindV3::Homing", EditConditionHides))
	FName HomingTargetSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover",
			  meta=(EditCondition="Motion==EMoverMotionKindV3::Homing", EditConditionHides))
	float HomingStrength = 1.0f;

};

USTRUCT(BlueprintType)
struct FDeliveryBeamConfigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float TickInterval = 0.f; // 0 = every tick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Range = 1500.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	float Radius = 0.f; // 0 = line trace

	// Optional lock-on
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	bool bLockOnTarget = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Beam")
	FName LockTargetSet = NAME_None;
};

// ============================================================
// Primitive spec
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryEventPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bUseKindDefaults = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitStarted = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitStopped = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitTick = true; // NEW (empfohlen, damit Tick nicht immer spammt)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitHit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	FGameplayTagContainer ExtraTags;
};


USTRUCT(BlueprintType)
struct FDeliveryFieldEventPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitEnter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitExit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Events")
	bool bEmitStay = false;
};

UENUM(BlueprintType)
enum class EDeliveryTagMatchModeV3 : uint8
{
	Exact,
	MatchesTag // hierarchical (GameplayTags MatchesTag)
};

USTRUCT(BlueprintType)
struct FDeliveryStopOnEventPolicyV3
{
	GENERATED_BODY()

	// Master switch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop")
	bool bEnabled = false;

	// Which event stops this primitive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled", EditConditionHides))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled", EditConditionHides))
	EDeliveryTagMatchModeV3 MatchMode = EDeliveryTagMatchModeV3::Exact;

	// Optional: require the event to come from the same delivery group
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled", EditConditionHides))
	bool bRequireSameGroup = true;

	// Optional: require the event to come from a specific emitter primitive id
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled", EditConditionHides))
	bool bRequireEmitterPrimitive = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled && bRequireEmitterPrimitive", EditConditionHides))
	FName EmitterPrimitiveId = NAME_None;

	// Optional: require extra tags on the event (Ev.Tags must contain all)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop", meta=(EditCondition="bEnabled", EditConditionHides))
	FGameplayTagContainer RequiredEventTags;
};


USTRUCT(BlueprintType)
struct FDeliveryPrimitiveSpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	EDeliveryKindV3 Kind = EDeliveryKindV3::InstantQuery;
	
	// ---------- Kind-specific config (direkt darunter!) ----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive",
			  meta=(EditCondition="Kind==EDeliveryKindV3::InstantQuery", EditConditionHides))
	FDeliveryInstantQueryConfigV3 InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive",
			  meta=(EditCondition="Kind==EDeliveryKindV3::Field", EditConditionHides))
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive",
			  meta=(EditCondition="Kind==EDeliveryKindV3::Mover", EditConditionHides))
	FDeliveryMoverConfigV3 Mover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive",
			  meta=(EditCondition="Kind==EDeliveryKindV3::Beam", EditConditionHides))
	FDeliveryBeamConfigV3 Beam;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryAnchorRefV3 Anchor;
	
	// Endformat: multiple motions allowed. Implementation currently applies only Motions[0].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	TArray<FDeliveryMotionSpecV3> Motions;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	EDeliveryMissingAnchorPolicyV3 OnMissingAnchor = EDeliveryMissingAnchorPolicyV3::FallbackToRoot;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryQueryPolicyV3 Query;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FName OutTargetSetName = NAME_None;

	// Debug override
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bOverrideDebugDraw = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug",
			  meta=(EditCondition="bOverrideDebugDraw", EditConditionHides))
	FDeliveryDebugDrawConfigV3 DebugDrawOverride;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryEventPolicyV3 Events;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive")
	FDeliveryStopOnEventPolicyV3 StopOnEvent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Primitive",
		meta=(EditCondition="Kind==EDeliveryKindV3::Field || Kind==EDeliveryKindV3::Beam", EditConditionHides))
	FDeliveryFieldEventPolicyV3 FieldEvents;

};

// ============================================================
// Group spec (composite)
// ============================================================

USTRUCT(BlueprintType)
struct FDeliverySpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName DeliveryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryAttachV3 Attach;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig", meta=(AdvancedDisplay))
	TObjectPtr<UDeliveryRigV3> Rig = nullptr;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Pose")
	EDeliveryPoseUpdatePolicyV3 PoseUpdatePolicy = EDeliveryPoseUpdatePolicyV3::EveryTick;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Pose",
			  meta=(EditCondition="PoseUpdatePolicy==EDeliveryPoseUpdatePolicyV3::Interval", EditConditionHides))
	float PoseUpdateInterval = 0.05f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FName OutTargetSetDefault = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryDebugDrawConfigV3 DebugDrawDefaults;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryBlackboardInitSpecV3 BlackboardInit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	FDeliveryOwnershipRulesV3 OwnershipRules;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Group")
	TArray<FDeliveryPrimitiveSpecV3> Primitives;
};



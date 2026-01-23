#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/Rig/DeliveryRigV3.h"
#include "DeliverySpecV3.generated.h"

// Typed configs FIRST (UHT requires complete types in UPROPERTY)

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

	// Enable/disable debug draw for this delivery instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bEnable = false;

	// How long debug shapes persist (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	float Duration = 2.0f;

	// Draw the evaluated shape (sphere/capsule/box/ray)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawShape = true;

	// Draw path/segment (e.g., mover sweep, instant ray)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawPath = true;

	// Draw hit points / impact normals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug")
	bool bDrawHits = true;
	
	// Draw rig root + emitter poses (if Rig is used)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug") bool bDrawRig = true;

	// Draw small axes arrows for rig poses
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug") bool bDrawRigAxes = true;

	// Visual size of rig points
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Debug") float RigPointSize = 12.f;

};

// Event-specific semantic tags (appended to HitTags depending on event type)
USTRUCT(BlueprintType)
struct FDeliveryEventHitTagsV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Started;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Stopped;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Hit;   // InstantQuery/Mover
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Enter; // Field/Beam
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Stay;  // Field/Beam
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Exit;  // Field/Beam
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|EventTags") FGameplayTagContainer Tick;  // optional heartbeat
};

// Per-emitter/per-instance overrides (index == EmitterIndex when Rig produces Emitters).
// This enables "one StartDelivery -> N different instances" deterministically.
USTRUCT(BlueprintType)
struct FDeliveryInstanceOverrideV3
{
	GENERATED_BODY()

	// If true, override Kind for this instance (allows mixing primitives within one start).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideKind = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideKind"))
	EDeliveryKindV3 Kind = EDeliveryKindV3::InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideShape = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideShape"))
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideQuery = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideQuery"))
	FDeliveryQueryPolicyV3 Query;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideStopPolicy = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideStopPolicy"))
	FDeliveryStopPolicyV3 StopPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideOutTargetSet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideOutTargetSet"))
	FName OutTargetSet = NAME_None;

	// Tag modifiers: appended to base tags
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	FGameplayTagContainer AddDeliveryTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	FGameplayTagContainer AddHitTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	FDeliveryEventHitTagsV3 AddEventHitTags;

	// Optional typed config overrides (only meaningful if Kind matches)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideInstantQuery = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideInstantQuery"))
	FDeliveryInstantQueryConfigV3 InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideField = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideField"))
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideMover = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideMover"))
	FDeliveryMoverConfigV3 Mover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides")
	bool bOverrideBeam = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Overrides", meta=(EditCondition="bOverrideBeam"))
	FDeliveryBeamConfigV3 Beam;
};


USTRUCT(BlueprintType)
struct FDeliverySpecV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryKindV3 Kind = EDeliveryKindV3::InstantQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = "Delivery";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryAttachV3 Attach;

	// Optional gameplay rig: if Nodes is empty, drivers will use Attach directly (backwards compatible).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryRigV3 Rig;

	// How often the rig pose should be evaluated (time-aware rigs).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryPoseUpdatePolicyV3 PoseUpdatePolicy = EDeliveryPoseUpdatePolicyV3::OnStart;

	// Used when PoseUpdatePolicy == Interval (0 => drivers may choose their own tick interval).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	float PoseUpdateInterval = 0.f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryQueryPolicyV3 Query;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryStopPolicyV3 StopPolicy;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryDebugDrawConfigV3 DebugDraw;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FGameplayTagContainer DeliveryTags;

	// Base semantic tags forwarded into DeliveryEventContextV3.HitTags for all events
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FGameplayTagContainer HitTags;

	// Additional per-event semantic tags (appended depending on event type)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryEventHitTagsV3 EventHitTags;

	// Writeback: delivery can write hit/inside targets into TargetStore for downstream Effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FName OutTargetSet = "Targets";

	// Optional per-emitter overrides. Array index corresponds to EmitterIndex when Rig produces Emitters.
	// If Rig produces N emitters and Overrides has >= N entries, each instance i uses Overrides[i].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TArray<FDeliveryInstanceOverrideV3> InstanceOverrides;

	// Optional overrides addressed by rig SpawnSlot.
	// SlotOverrides[SpawnSlot] is applied first, then InstanceOverrides[EmitterIndex].
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TArray<FDeliveryInstanceOverrideV3> SlotOverrides;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryInstantQueryConfigV3 InstantQuery;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryMoverConfigV3 Mover;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Mover")
	bool bKinematicFromRig = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryBeamConfigV3 Beam;
};




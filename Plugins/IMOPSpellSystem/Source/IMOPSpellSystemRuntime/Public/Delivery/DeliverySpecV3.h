#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Delivery/DeliveryTypesV3.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryShapeV3 Shape;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryQueryPolicyV3 Query;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryStopPolicyV3 StopPolicy;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	FDeliveryDebugDrawConfigV3 DebugDraw;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") FGameplayTagContainer DeliveryTags;

	// Base semantic tags forwarded into DeliveryEventContextV3.HitTags for all events
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") FGameplayTagContainer HitTags;

	// Additional per-event semantic tags (appended depending on event type)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") FDeliveryEventHitTagsV3 EventHitTags;

	// Writeback: delivery can write hit/inside targets into TargetStore for downstream Effects
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") FName OutTargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") FDeliveryInstantQueryConfigV3 InstantQuery;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryFieldConfigV3 Field;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryMoverConfigV3 Mover;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryBeamConfigV3 Beam;
};




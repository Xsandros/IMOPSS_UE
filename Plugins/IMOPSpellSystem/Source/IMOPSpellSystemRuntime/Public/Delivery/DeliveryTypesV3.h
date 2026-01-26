#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DeliveryTypesV3.generated.h"

// ============================================================
// Handle
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryHandleV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FGuid RuntimeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 InstanceIndex = 0;

	bool IsValid() const
	{
		return RuntimeGuid.IsValid() && DeliveryId != NAME_None;
	}

	bool operator==(const FDeliveryHandleV3& Other) const
	{
		return RuntimeGuid == Other.RuntimeGuid
			&& DeliveryId == Other.DeliveryId
			&& InstanceIndex == Other.InstanceIndex;
	}
};

FORCEINLINE uint32 GetTypeHash(const FDeliveryHandleV3& H)
{
	uint32 X = ::GetTypeHash(H.RuntimeGuid);
	X = HashCombineFast(X, ::GetTypeHash(H.DeliveryId));
	X = HashCombineFast(X, ::GetTypeHash(H.InstanceIndex));
	return X;
}

// ============================================================
// Core enums
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryKindV3 : uint8
{
	InstantQuery,
	Field,
	Mover,
	Beam
};

UENUM(BlueprintType)
enum class EDeliveryPoseUpdatePolicyV3 : uint8
{
	OnStart,
	EveryTick,
	Interval
};

UENUM(BlueprintType)
enum class EDeliveryStopReasonV3 : uint8
{
	Manual,
	DurationElapsed,
	OnFirstHit,
	OnEvent,
	OwnerDestroyed,
	SpellEnded,
	Expired,
	Failed
};

// ============================================================
// Shapes
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryShapeV3 : uint8
{
	Sphere,
	Box,
	Capsule,
	Ray
};

USTRUCT(BlueprintType)
struct FDeliveryShapeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	EDeliveryShapeV3 Kind = EDeliveryShapeV3::Sphere;

	// Sphere/Capsule
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	float Radius = 30.f;

	// Capsule
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	float HalfHeight = 60.f;

	// Box
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	FVector Extents = FVector(30.f, 30.f, 30.f);
};

// ============================================================
// Query policy
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryQueryModeV3 : uint8
{
	Overlap,
	Sweep,
	LineTrace
};

USTRUCT(BlueprintType)
struct FDeliveryQueryPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryModeV3 Mode = EDeliveryQueryModeV3::LineTrace;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	FName CollisionProfile = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	bool bIgnoreCaster = true;
};

// ============================================================
// Stop policy (data-only; enforcement can be done by driver or group)
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryStopPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop")
	bool bStopOnFirstHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop")
	float MaxDurationSeconds = 0.f; // 0 = unlimited

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Stop")
	int32 MaxHits = 0; // 0 = unlimited
};

// ============================================================
// Attach (group root space)
// ============================================================

UENUM(BlueprintType)
enum class EDeliveryAttachModeV3 : uint8
{
	World,
	Caster,
	CasterSocket,
	TargetActor // (future: target set)
};

USTRUCT(BlueprintType)
struct FDeliveryAttachV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	EDeliveryAttachModeV3 Mode = EDeliveryAttachModeV3::Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FTransform LocalOffset = FTransform::Identity;

	// Optional: for TargetActor mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	TWeakObjectPtr<AActor> TargetActor;
};

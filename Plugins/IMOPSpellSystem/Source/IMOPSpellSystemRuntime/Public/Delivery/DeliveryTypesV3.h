#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include "DeliveryTypesV3.generated.h"

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
enum class EDeliveryStopReasonV3 : uint8
{
	Manual,
	SpellEnded,
	Expired,
	OnFirstHit,
	Failed
};

UENUM(BlueprintType)
enum class EDeliveryPoseUpdatePolicyV3 : uint8
{
	EveryTick,
	Interval,
	OnStart
};

UENUM(BlueprintType)
enum class EDeliveryQueryModeV3 : uint8
{
	LineTrace,
	Sweep,
	Overlap
};

UENUM(BlueprintType)
enum class EDeliveryShapeV3 : uint8
{
	Sphere,
	Box,
	Capsule,
	Ray
};

UENUM(BlueprintType)
enum class EDeliveryAttachModeV3 : uint8
{
	Caster,
	World,
	CasterSocket,
	TargetActor
};

// ============================================================
// Handle / identity
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

	friend bool operator==(const FDeliveryHandleV3& A, const FDeliveryHandleV3& B)
	{
		return A.RuntimeGuid == B.RuntimeGuid && A.DeliveryId == B.DeliveryId && A.InstanceIndex == B.InstanceIndex;
	}

	friend uint32 GetTypeHash(const FDeliveryHandleV3& H)
	{
		uint32 X = ::GetTypeHash(H.RuntimeGuid);
		X = HashCombine(X, ::GetTypeHash(H.DeliveryId));
		X = HashCombine(X, ::GetTypeHash(H.InstanceIndex));
		return X;
	}
};

// ============================================================
// Shape + query policy
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryShapeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	EDeliveryShapeV3 Kind = EDeliveryShapeV3::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	float Radius = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	FVector Extents = FVector(25.f, 25.f, 25.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape")
	float HalfHeight = 50.f;
};

USTRUCT(BlueprintType)
struct FDeliveryQueryPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryModeV3 Mode = EDeliveryQueryModeV3::Sweep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	FName CollisionProfile = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	bool bIgnoreCaster = true;
};

// ============================================================
// Attach
// ============================================================

USTRUCT(BlueprintType)
struct FDeliveryAttachV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	EDeliveryAttachModeV3 Mode = EDeliveryAttachModeV3::Caster;

	// For CasterSocket mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FName SocketName = NAME_None;

	// For TargetActor mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	TWeakObjectPtr<AActor> TargetActor;

	// Local offset relative to chosen base
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FTransform LocalOffset = FTransform::Identity;
};

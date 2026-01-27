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
	DurationElapsed,
	OnFirstHit,
	OnEvent,
	OwnerDestroyed,
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
};

// IMPORTANT: TMap key hashing (must not rely on GetTypeHash(FGuid) visibility)
FORCEINLINE uint32 GetTypeHash(const FDeliveryHandleV3& H)
{
	// Local, self-contained mixing (no engine overload dependencies)
	auto HashU32 = [](uint32 X) -> uint32
	{
		X ^= X >> 16;
		X *= 0x7feb352du;
		X ^= X >> 15;
		X *= 0x846ca68bu;
		X ^= X >> 16;
		return X;
	};

	auto HashCombineLocal = [&](uint32 A, uint32 B) -> uint32
	{
		return HashU32(A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)));
	};

	uint32 X = 0;
	// Hash Guid components directly
	X = HashCombineLocal(X, HashU32((uint32)H.RuntimeGuid.A));
	X = HashCombineLocal(X, HashU32((uint32)H.RuntimeGuid.B));
	X = HashCombineLocal(X, HashU32((uint32)H.RuntimeGuid.C));
	X = HashCombineLocal(X, HashU32((uint32)H.RuntimeGuid.D));

	// FName hashing: avoid ::GetTypeHash(FName) if you want, but it is usually safe.
	// We'll still avoid it to keep this self-contained:
	X = HashCombineLocal(X, HashU32((uint32)H.DeliveryId.GetComparisonIndex().ToUnstableInt()));
	X = HashCombineLocal(X, HashU32((uint32)H.DeliveryId.GetNumber()));

	X = HashCombineLocal(X, HashU32((uint32)H.InstanceIndex));
	return X;
}

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FName SocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	TWeakObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Attach")
	FTransform LocalOffset = FTransform::Identity;
};

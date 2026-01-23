#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DeliveryTypesV3.generated.h"

UENUM(BlueprintType)
enum class EDeliveryKindV3 : uint8
{
	InstantQuery,
	Field,
	Mover,
	Beam
};

UENUM(BlueprintType)
enum class EDeliveryAttachKindV3 : uint8
{
	World,
	Caster,
	TargetSet, // contract-friendly (no actor ptr in spec)
	ActorRef,  // contract-friendly (resolve via name/ref later)
	Socket
};

USTRUCT(BlueprintType)
struct FDeliveryAttachV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	EDeliveryAttachKindV3 Kind = EDeliveryAttachKindV3::Caster;

	// Contract-friendly references (resolved at runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName TargetSetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName ActorRefName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName SocketName = NAME_None;

	// World anchor (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FTransform WorldTransform = FTransform::Identity;
};

UENUM(BlueprintType)
enum class EDeliveryShapeV3 : uint8
{
	Sphere,
	Capsule,
	Box,
	Ray
};

USTRUCT(BlueprintType)
struct FDeliveryShapeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	EDeliveryShapeV3 Kind = EDeliveryShapeV3::Sphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	float Radius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	float HalfHeight = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FVector Extents = FVector::ZeroVector;
};

UENUM(BlueprintType)
enum class EDeliveryQueryModeV3 : uint8
{
	Overlap,
	Sweep,
	LineTrace
};

UENUM(BlueprintType)
enum class EDeliveryPoseUpdatePolicyV3 : uint8
{
	// Evaluate pose once on Start, then keep it fixed.
	OnStart,

	// Evaluate pose every frame tick (time-aware rigs).
	EveryTick,

	// Evaluate pose on a fixed interval (e.g. Field/Beam evaluation ticks).
	Interval
};


USTRUCT(BlueprintType)
struct FDeliveryQueryPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	EDeliveryQueryModeV3 Mode = EDeliveryQueryModeV3::Overlap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName CollisionProfile = "OverlapAll";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bIgnoreCaster = true;
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

USTRUCT(BlueprintType)
struct FDeliveryStopPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	bool bStopOnFirstHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	float MaxDuration = 0.f; // 0 => infinite

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FGameplayTag StopOnEventTag;
};

USTRUCT(BlueprintType)
struct FDeliveryHandleV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FGuid RuntimeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	FName DeliveryId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Delivery")
	int32 InstanceIndex = 0;

	bool IsValid() const { return RuntimeGuid.IsValid() && DeliveryId != NAME_None; }

	friend bool operator==(const FDeliveryHandleV3& A, const FDeliveryHandleV3& B)
	{
		return A.RuntimeGuid == B.RuntimeGuid && A.DeliveryId == B.DeliveryId && A.InstanceIndex == B.InstanceIndex;
	}
};

FORCEINLINE uint32 GetTypeHash(const FDeliveryHandleV3& H)
{
	return HashCombine(HashCombine(GetTypeHash(H.RuntimeGuid), GetTypeHash(H.DeliveryId)), GetTypeHash(H.InstanceIndex));
}

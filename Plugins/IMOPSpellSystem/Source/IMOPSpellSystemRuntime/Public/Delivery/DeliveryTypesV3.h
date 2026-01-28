#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Misc/Guid.h"
#include "Templates/TypeHash.h"
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
enum class EDeliveryAnchorFollowModeV3 : uint8
{
	FreezeOnPlace UMETA(DisplayName="Freeze On Place"),
	Follow         UMETA(DisplayName="Follow Anchor"),
};

UENUM(BlueprintType)
enum class EDeliveryQueryFilterModeV3 : uint8
{
	ByProfile   UMETA(DisplayName="By Collision Profile"),
	ByChannel   UMETA(DisplayName="By Trace Channel"),
	ByObjectType UMETA(DisplayName="By Object Type"),
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
		return A.RuntimeGuid == B.RuntimeGuid
			&& A.DeliveryId == B.DeliveryId
			&& A.InstanceIndex == B.InstanceIndex;
	}
};

// Self-contained hash (does NOT rely on engine GetTypeHash(FGuid/FName) visibility)
FORCEINLINE uint32 GetTypeHash(const FDeliveryHandleV3& H)
{
	auto Mix32 = [](uint32 X) -> uint32
	{
		X ^= X >> 16;
		X *= 0x7feb352du;
		X ^= X >> 15;
		X *= 0x846ca68bu;
		X ^= X >> 16;
		return X;
	};

	auto CombineLocal = [&](uint32 A, uint32 B) -> uint32
	{
		// Same spirit as UE HashCombine, but fully local
		return Mix32(A ^ (B + 0x9e3779b9u + (A << 6) + (A >> 2)));
	};

	uint32 X = 0;

	// FGuid: hash components directly (A/B/C/D are uint32)
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.RuntimeGuid.A)));
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.RuntimeGuid.B)));
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.RuntimeGuid.C)));
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.RuntimeGuid.D)));

	// FName: use stable parts (ComparisonIndex + Number)
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.DeliveryId.GetComparisonIndex().ToUnstableInt())));
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.DeliveryId.GetNumber())));

	// InstanceIndex
	X = CombineLocal(X, Mix32(static_cast<uint32>(H.InstanceIndex)));

	return X;
}


USTRUCT(BlueprintType)
struct FDeliveryPrimitiveHandleV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Group;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = NAME_None;

	friend bool operator==(const FDeliveryPrimitiveHandleV3& A, const FDeliveryPrimitiveHandleV3& B)
	{
		return A.Group == B.Group && A.PrimitiveId == B.PrimitiveId;
	}
};

FORCEINLINE uint32 GetTypeHash(const FDeliveryPrimitiveHandleV3& H)
{
	// Now safe because FDeliveryPrimitiveHandleV3 is fully defined above
	const uint32 A = GetTypeHash(H.Group);
	const uint32 B = static_cast<uint32>(H.PrimitiveId.GetComparisonIndex().ToUnstableInt()) ^ (static_cast<uint32>(H.PrimitiveId.GetNumber()) * 16777619u);
	return HashCombine(A, B);
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape",
			  meta=(EditCondition="Kind==EDeliveryShapeV3::Sphere", EditConditionHides))
	float Radius = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape",
			  meta=(EditCondition="Kind==EDeliveryShapeV3::Box", EditConditionHides))
	FVector Extents = FVector(50);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape",
			  meta=(EditCondition="Kind==EDeliveryShapeV3::Capsule", EditConditionHides))
	float CapsuleRadius = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape",
			  meta=(EditCondition="Kind==EDeliveryShapeV3::Capsule", EditConditionHides))
	float HalfHeight = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Shape",
			  meta=(EditCondition="Kind==EDeliveryShapeV3::Ray", EditConditionHides))
	float RayLength = 1000.f;
};

USTRUCT(BlueprintType)
struct FDeliveryQueryPolicyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryModeV3 Mode = EDeliveryQueryModeV3::Sweep;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query")
	EDeliveryQueryFilterModeV3 FilterMode = EDeliveryQueryFilterModeV3::ByChannel;

	// Profile dropdown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query",
			  meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByProfile", EditConditionHides))
	FCollisionProfileName CollisionProfile;

	// Channel dropdown
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query",
			  meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByChannel", EditConditionHides))
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Visibility;

	// Object types
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Query",
			  meta=(EditCondition="FilterMode==EDeliveryQueryFilterModeV3::ByObjectType", EditConditionHides))
	TArray<TEnumAsByte<ECollisionChannel>> ObjectTypes;

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


#pragma once

#include "CoreMinimal.h"

#include "Delivery/DeliveryTypesV3.h"

// Forward decl (keep includes light)
struct FSpellExecContextV3;

#include "DeliveryContextV3.generated.h"

/**
 * Per-primitive runtime context snapshot.
 * - Stored inside UDeliveryGroupRuntimeV3::PrimitiveCtxById and updated when rig/driver motion changes.
 */
USTRUCT(BlueprintType)
struct FDeliveryContextV3
{
	GENERATED_BODY()

	// Identity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 PrimitiveIndex = 0;

	// Who/when
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	TWeakObjectPtr<AActor> Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	float StartTimeSeconds = 0.f;

	// Deterministic per primitive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 Seed = 0;

	// Spec copy (effective spec for this primitive)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	struct FDeliveryPrimitiveSpecV3 Spec;

	// Poses
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform AnchorPoseWS = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform FinalPoseWS = FTransform::Identity;
};

#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "DeliveryContextV3.generated.h"

/**
 * Per-primitive runtime context (created by subsystem, updated when rig updates).
 * This is passed into drivers on Start, and drivers can read live versions from GroupRuntime.
 */
USTRUCT(BlueprintType)
struct FDeliveryContextV3
{
	GENERATED_BODY()

	// Group identity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FDeliveryHandleV3 GroupHandle;

	// Primitive identity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 PrimitiveIndex = 0;

	// Authoring spec snapshot for this primitive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FDeliveryPrimitiveSpecV3 Spec;

	// Caster snapshot (weak)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	TWeakObjectPtr<AActor> Caster;

	// Time/seed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	float StartTimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 Seed = 0;

	// Anchor pose (from Rig/Attach)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform AnchorPoseWS = FTransform::Identity;

	// Final pose after motion (future: per-primitive motion stacks)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform FinalPoseWS = FTransform::Identity;
};

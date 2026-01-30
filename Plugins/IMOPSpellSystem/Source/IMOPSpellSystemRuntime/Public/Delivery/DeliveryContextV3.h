#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h" // Required (FDeliveryPrimitiveSpecV3 is a value member)

#include "DeliveryContextV3.generated.h"

class AActor;

/**
 * Per-primitive runtime context snapshot.
 * Stored inside UDeliveryGroupRuntimeV3::PrimitiveCtxById and updated when rig/driver motion changes.
 */
USTRUCT(BlueprintType)
struct FDeliveryContextV3
{
	GENERATED_BODY()

	// =========================
	// Identity
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 PrimitiveIndex = 0;

	// =========================
	// Who / when
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	TWeakObjectPtr<AActor> Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	float StartTimeSeconds = 0.f;

	// =========================
	// Determinism
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	int32 Seed = 0;

	// =========================
	// Spec (effective per-primitive)
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FDeliveryPrimitiveSpecV3 Spec;

	// =========================
	// Poses (world space)
	// =========================

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform AnchorPoseWS = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Ctx")
	FTransform FinalPoseWS = FTransform::Identity;
	
	// --- Anchor freeze cache (runtime only) ---
	UPROPERTY(Transient)
	bool bAnchorFrozenWS = false;

	UPROPERTY(Transient)
	FTransform FrozenAnchorWS = FTransform::Identity;

};

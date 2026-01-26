#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "DeliveryContextV3.generated.h"

// Runtime context for ONE primitive inside a composite group.
USTRUCT(BlueprintType)
struct FDeliveryContextV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = "P0";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 PrimitiveIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 EmitterIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 SpawnSlot = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	float StartTimeSeconds = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 Seed = 0;

	// Snapshot of spec data for this primitive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryPrimitiveSpecV3 Spec;

	// Cached anchor pose for this primitive (updated by group rig policy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FTransform AnchorPoseWS = FTransform::Identity;

	// Cached final pose (Anchor + local offsets + motion), if you want it.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FTransform FinalPoseWS = FTransform::Identity;

	// Runtime-resolved references (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TWeakObjectPtr<AActor> Caster;
};

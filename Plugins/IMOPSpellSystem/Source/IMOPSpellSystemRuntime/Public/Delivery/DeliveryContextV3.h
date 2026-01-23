#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "DeliveryContextV3.generated.h"

USTRUCT(BlueprintType)
struct FDeliveryContextV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle;

	// Identifies which sub-primitive / rig emitter this delivery instance represents.
	// Stable per spawned instance, used for logs and propagated into delivery events.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = "P0";

	// When spawned from a rig multi-emitter evaluation, this indicates which emitter index this instance uses.
	// -1 means "root" / not emitter-driven.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 EmitterIndex = -1;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliverySpecV3 Spec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TWeakObjectPtr<AActor> Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	float StartTime = 0.f;
	
	// Cached pose derived from Rig/Attach according to PoseUpdatePolicy
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FTransform CachedPose = FTransform::Identity;

	// Pose update accumulator (for Interval policy)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	float PoseAccum = 0.f;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 Seed = 0;
};

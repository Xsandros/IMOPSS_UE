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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliverySpecV3 Spec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TWeakObjectPtr<AActor> Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	float StartTime = 0.f;

	// For rig-driven multi-emitter deliveries: which emitter pose this instance should use.
	// INDEX_NONE => use RigOut.Root.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery") 
	int32 EmitterIndex = INDEX_NONE;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 Seed = 0;
};

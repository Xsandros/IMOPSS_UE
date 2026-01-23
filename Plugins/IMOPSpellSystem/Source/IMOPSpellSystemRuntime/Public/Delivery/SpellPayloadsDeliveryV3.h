#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "SpellPayloadsDeliveryV3.generated.h"

USTRUCT(BlueprintType)
struct FPayload_DeliveryStartV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliverySpecV3 Spec;
};

USTRUCT(BlueprintType)
struct FPayload_DeliveryStopV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = NAME_None;

	// Stop exactly one spawned primitive instance.
	// If DeliveryId is None, it will search across all active deliveries in this runtime.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUseHandle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUsePrimitiveId = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryStopReasonV3 Reason = EDeliveryStopReasonV3::Manual;

};

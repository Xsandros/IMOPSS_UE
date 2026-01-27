#pragma once

#include "CoreMinimal.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"

#include "SpellPayloadsDeliveryV3.generated.h"

/**
 * Payload: Start a composite delivery group.
 */
USTRUCT(BlueprintType)
struct FPayload_DeliveryStartV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliverySpecV3 Spec;
};

/**
 * Payload: Stop delivery by:
 * - Handle (preferred when known), OR
 * - DeliveryId (stop all instances of that DeliveryId for this runtime), OR
 * - DeliveryId + PrimitiveId (stop a single primitive in all matching instances)
 */
USTRUCT(BlueprintType)
struct FPayload_DeliveryStopV3
{
	GENERATED_BODY()

	// If true, use Handle
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUseHandle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryPrimitiveHandleV3 PrimitiveHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUsePrimitiveHandle = false;


	// Fallback stop by id
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = NAME_None;

	// Optional stop by primitive
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUsePrimitiveId = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryStopReasonV3 Reason = EDeliveryStopReasonV3::Manual;
};

#pragma once

#include "CoreMinimal.h"
#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliverySpecV3.h"
#include "SpellPayloadsDeliveryV3.generated.h"

/**
 * Action Payloads for Delivery (Phase 4B Composite-first)
 *
 * Notes:
 * - Start uses full FDeliverySpecV3 (composite group).
 * - Stop supports:
 *   - explicit Handle
 *   - DeliveryId (stop all instances with that id in the runtime)
 *   - DeliveryId + PrimitiveId (stop one primitive in all matching groups)
 */

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

	// Stop by Handle (preferred when stored)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUseHandle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle;

	// Stop by DeliveryId (within runtime)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = NAME_None;

	// Optional: stop a specific primitive (requires DeliveryId)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUsePrimitiveId = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryStopReasonV3 Reason = EDeliveryStopReasonV3::Manual;
};

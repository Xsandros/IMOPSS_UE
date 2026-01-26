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
	FDeliveryCompositeSpecV3 Spec;
};

USTRUCT(BlueprintType)
struct FPayload_DeliveryStopV3
{
	GENERATED_BODY()

	// If set: stop all groups & primitives with this DeliveryId in this runtime
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName DeliveryId = NAME_None;

	// If set with DeliveryId: stop only a specific group (per cast)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 GroupIndex = INDEX_NONE;

	// If set: stop only primitives with this PrimitiveId (optionally restricted by DeliveryId/GroupIndex)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = NAME_None;

	// Exact primitive instance
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	bool bUseHandle = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryStopReasonV3 Reason = EDeliveryStopReasonV3::Manual;
};

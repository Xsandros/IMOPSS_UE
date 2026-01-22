#pragma once

#include "CoreMinimal.h"
#include "Delivery/Rig/DeliveryRigNodeV3.h"
#include "DeliveryRigV3.generated.h"

USTRUCT(BlueprintType)
struct FDeliveryRigV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FDeliveryRigNodeV3> Nodes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	int32 RootIndex = 0;

	bool IsEmpty() const { return Nodes.Num() == 0; }
};

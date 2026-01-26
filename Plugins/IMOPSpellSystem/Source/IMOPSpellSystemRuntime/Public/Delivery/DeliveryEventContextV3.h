#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "Delivery/DeliveryTypesV3.h"

#include "DeliveryEventContextV3.generated.h"

/**
 * Future-proof delivery event payload (Phase 12+ can use it for VFX/SFX).
 * For now it is optional scaffolding; we can emit this later via SpellTrace or EventBus.
 */
USTRUCT(BlueprintType)
struct FDeliveryEventContextV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FName PrimitiveId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	TWeakObjectPtr<AActor> Instigator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	TWeakObjectPtr<AActor> HitActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FVector HitNormal = FVector::UpVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FGameplayTagContainer Tags;
};

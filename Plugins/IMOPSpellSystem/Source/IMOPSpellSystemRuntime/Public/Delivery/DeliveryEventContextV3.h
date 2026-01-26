#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/HitResult.h"

#include "Delivery/DeliveryTypesV3.h"

#include "DeliveryEventContextV3.generated.h"

/**
 * Payload for delivery-related events (Phase 4+).
 * We keep it flexible and purely data-driven so later phases (Presentation/Net/Trace)
 * can reuse it without rewrites.
 */
USTRUCT(BlueprintType)
struct FDeliveryEventContextV3
{
	GENERATED_BODY()

	// Which group/primitive produced this event
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FName PrimitiveId = NAME_None;

	// Optional semantic tags for the event (in addition to the SpellEvent tag)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FGameplayTagContainer DeliveryTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FGameplayTagContainer HitTags;

	// Common payload fields
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FVector OriginWS = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FVector DirectionWS = FVector::ForwardVector;

	// For hit/trace events
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	TArray<FHitResult> Hits;

	// Convenience: first hit actor list (optional)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	TArray<TWeakObjectPtr<AActor>> HitActors;

	// Optional: where hits were written (if any)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Event")
	FName OutTargetSetName = NAME_None;
};

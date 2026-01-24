#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Delivery/DeliveryTypesV3.h"
#include "DeliveryEventContextV3.generated.h"

UENUM(BlueprintType)
enum class EDeliveryEventTypeV3 : uint8
{
	Started,
	Stopped,
	Hit,
	Enter,
	Stay,
	Exit,
	Tick
};

USTRUCT(BlueprintType)
struct FDeliveryHitV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TWeakObjectPtr<AActor> Target;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FVector Normal = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FGameplayTag HitZoneTag;
};

USTRUCT(BlueprintType)
struct FDeliveryEventContextV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryEventTypeV3 Type = EDeliveryEventTypeV3::Hit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FDeliveryHandleV3 Handle = {};

	// Identifies which sub-primitive/node produced this event.
	// For single-primitive deliveries, use a stable default like "P0".
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FName PrimitiveId = "P0";

	// Emitter index that produced this event (-1 for root/single primitive).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 EmitterIndex = -1;

	// Rig slot used for this primitive (0=default). Useful for per-slot authoring.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	int32 SpawnSlot = 0;

	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	EDeliveryStopReasonV3 StopReminder = EDeliveryStopReasonV3::Manual;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FGameplayTagContainer DeliveryTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	FGameplayTagContainer HitTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TWeakObjectPtr<AActor> Caster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery")
	TArray<FDeliveryHitV3> Hits;
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Delivery/DeliveryTypesV3.h" // <-- NEU (für FDeliveryHandleV3)
#include "SpellEventV3.generated.h"

/**
 * Common event envelope used by SpellEventBusSubsystemV3.
 * Payloads live in Data as FInstancedStruct (typed, optional).
 */
USTRUCT(BlueprintType)
struct FSpellEventV3
{
    GENERATED_BODY()

    // Which spell runtime produced the event
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FGuid RuntimeGuid;

    // Tag describing the event (e.g. Spell.Cast.Start, Delivery.Hit, Spell.End, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FGameplayTag EventTag;

    // Common optional sender info (used by runtime filters/logging)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    TWeakObjectPtr<AActor> Caster;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    TWeakObjectPtr<UObject> Sender;

    // Optional instigator (sometimes differs from caster)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    TWeakObjectPtr<UObject> Instigator;

    // Optional numeric data (lightweight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    float Magnitude = 0.f;

    // Optional generic tags (lightweight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FGameplayTagContainer Tags;

    // Optional typed payload (DeliveryEventContextV3, RelationResolved, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FInstancedStruct Data;

    // Optional debug timing
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    int32 FrameNumber = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    float TimeSeconds = 0.f;
    
    // ===== Optional Delivery identity (Composite Deliveries) =====
    // If DeliveryHandle.DeliveryId != None, this event was produced by a delivery group.
    // If DeliveryPrimitiveId != None, it came from that specific primitive node.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FDeliveryHandleV3 DeliveryHandle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FName DeliveryPrimitiveId = NAME_None;
};

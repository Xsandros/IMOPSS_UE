#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SpellEventV3.generated.h"

/**
 * Common event envelope used by SpellEventBusSubsystemV3.
 * Payloads are intentionally NOT embedded here (Phase 4B compile-first).
 * Later phases can add optional payload references via separate channels (e.g. Trace, DeliveryEventContext).
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

    // Optional sender (caster/owner/etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    TWeakObjectPtr<AActor> Instigator;

    // Optional numeric data (lightweight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    float Magnitude = 0.f;

    // Optional generic tags (lightweight)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Event")
    FGameplayTagContainer Tags;
};

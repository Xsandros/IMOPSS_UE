#pragma once

#include "CoreMinimal.h"
#include "Spec/SpellActionV3.h"
#include "Events/SpellTriggerMatcherV3.h"
#include "SpellPhaseV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellPhaseV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Phase")
    FName PhaseId = NAME_None;

    // Trigger that activates this phase
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Phase")
    FSpellTriggerMatcherV3 Trigger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Phase")
    TArray<FSpellActionV3> Actions;
};

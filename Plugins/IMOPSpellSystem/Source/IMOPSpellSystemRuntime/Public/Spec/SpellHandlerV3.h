#pragma once

#include "CoreMinimal.h"
#include "Spec/SpellActionV3.h"
#include "Events/SpellTriggerMatcherV3.h"
#include "SpellHandlerV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellHandlerV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Handler")
    FName HandlerId = NAME_None;

    // Which events this handler reacts to
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Handler")
    FSpellTriggerMatcherV3 Trigger;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Handler")
    TArray<FSpellActionV3> Actions;
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SpellTriggerMatcherV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellTriggerMatcherV3
{
    GENERATED_BODY()

    // If true, use ExactTag match. Otherwise use TagQuery.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Trigger")
    bool bUseExactTag = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Trigger", meta = (EditCondition = "bUseExactTag"))
    FGameplayTag ExactTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Trigger", meta = (EditCondition = "!bUseExactTag"))
    FGameplayTagQuery TagQuery;

    bool Matches(const FGameplayTag& InTag) const
    {
        if (!InTag.IsValid())
        {
            return false;
        }

        if (bUseExactTag)
        {
            return ExactTag.IsValid() ? (InTag == ExactTag) : false;
        }

        // TagQuery matches against a container
        FGameplayTagContainer C;
        C.AddTag(InTag);
        return TagQuery.Matches(C);
    }
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Spec/SpellPhaseV3.h"
#include "Spec/SpellHandlerV3.h"
#include "SpellSpecV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellSpecV3 : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Spec")
    FName SpellId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Spec")
    TArray<FSpellPhaseV3> Phases;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Spec")
    TArray<FSpellHandlerV3> Handlers;
};

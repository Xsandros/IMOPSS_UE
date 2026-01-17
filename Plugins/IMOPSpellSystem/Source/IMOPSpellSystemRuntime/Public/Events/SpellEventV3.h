#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "SpellEventV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellEventV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Event")
    FGameplayTag EventTag;

    // Optional structured data (final-form)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Event")
    FInstancedStruct Data;

    UPROPERTY(BlueprintReadWrite, Category = "Spell|Event")
    TObjectPtr<AActor> Caster = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Spell|Event")
    TObjectPtr<UObject> Sender = nullptr;

    UPROPERTY(BlueprintReadWrite, Category = "Spell|Event")
    int32 FrameNumber = 0;

    UPROPERTY(BlueprintReadWrite, Category = "Spell|Event")
    float TimeSeconds = 0.f;
    
    UPROPERTY(BlueprintReadWrite, Category = "Spell|Event")
    FGuid RuntimeGuid;
};

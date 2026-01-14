#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Effects/EffectTypesV3.h"
#include "SpellStatusTypesV3.generated.h"

USTRUCT(BlueprintType)
struct FActiveStatusV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	int32 Stacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	float TimeRemaining = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	FEffectContextV3 AppliedContext;
};

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Effects/EffectTypesV3.h"
#include "SpellStatusDefinitionV3.generated.h"

UENUM(BlueprintType)
enum class EStatusStackPolicyV3 : uint8
{
	RefreshDuration,
	AddStacks,
	Replace,
	IgnoreIfPresent
};

USTRUCT(BlueprintType)
struct FSpellStatusDefinitionV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	float DefaultDuration = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	int32 MaxStacks = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	EStatusStackPolicyV3 StackPolicy = EStatusStackPolicyV3::RefreshDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Status")
	float TickInterval = 0.f; // reserved for later periodic effects
};

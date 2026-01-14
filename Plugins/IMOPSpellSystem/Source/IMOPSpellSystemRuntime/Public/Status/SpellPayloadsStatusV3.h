#pragma once

#include "CoreMinimal.h"
#include "Status/SpellStatusDefinitionV3.h"
#include "SpellPayloadsStatusV3.generated.h"

USTRUCT(BlueprintType)
struct FPayload_ApplyStatusV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FSpellStatusDefinitionV3 Definition;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	float DurationOverride = -1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	int32 StacksOverride = 0;
};

USTRUCT(BlueprintType)
struct FPayload_RemoveStatusV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FGameplayTag StatusTag;
};

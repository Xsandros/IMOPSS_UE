#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingTypesV3.h" // for ETargetRelationV3

#include "SpellRelationTypesV3.generated.h"

UENUM(BlueprintType)
enum class ESpellForcedRelationV3 : uint8
{
	None,
	ForceAlly,
	ForceEnemy,
	ForceNeutral
};

USTRUCT(BlueprintType)
struct FSpellRelationRulePairV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag B;
};

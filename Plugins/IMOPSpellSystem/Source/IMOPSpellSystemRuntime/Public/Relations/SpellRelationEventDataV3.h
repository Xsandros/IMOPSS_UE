#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Targeting/TargetingTypesV3.h"
#include "SpellRelationEventDataV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellEventData_RelationResolvedV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ETargetRelationV3 Relation = ETargetRelationV3::Any;

	// One of Spell.Relation.Reason.*
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ReasonTag;

	// Optional: if a rule matched, which pair contributed (kept lightweight)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag MatchedA;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag MatchedB;
};

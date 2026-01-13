#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Targeting/TargetingTypesV3.h"
#include "TargetingGameHooksV3.generated.h"

struct FSpellExecContextV3;

UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UTargetingGameHooksV3 : public UInterface
{
	GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API ITargetingGameHooksV3
{
	GENERATED_BODY()

public:
	// Relation zwischen Caster (aus Ctx) und Candidate
	virtual ETargetRelationV3 GetRelation(const FSpellExecContextV3& Ctx, const AActor* Candidate) const = 0;

	// “HasTag” Filter
	virtual bool HasGameplayTag(const AActor* Candidate, FGameplayTag Tag) const = 0;

	// “HasStatus” Filter (Phase 2 kann default false sein)
	virtual bool HasStatusTag(const AActor* Candidate, FGameplayTag StatusTag) const = 0;

	// “LineOfSight” Filter
	virtual bool HasLineOfSight(const FSpellExecContextV3& Ctx, const FVector& Origin, const AActor* Candidate) const = 0;
};

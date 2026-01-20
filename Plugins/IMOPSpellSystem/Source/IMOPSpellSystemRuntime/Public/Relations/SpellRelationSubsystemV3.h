#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Targeting/TargetingTypesV3.h"
#include "Relations/SpellRelationTypesV3.h"
#include "GameplayTagContainer.h"

#include "SpellRelationSubsystemV3.generated.h"

class USpellRelationRulesAssetV3;

UENUM(BlueprintType)
enum class EAffiliationScopeMatchV3 : uint8
{
	// Tags match if they share the exact same leaf tag under the scope root (recommended)
	ExactTag,

	// Tags match if both have *any* tag under the scope root (rarely useful; usually too broad)
	AnyUnderRoot
};

USTRUCT(BlueprintType)
struct FAffiliationScopeV3
{
	GENERATED_BODY()

	// Example roots: Affiliation.Party, Affiliation.Guild, Affiliation.Faction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relations")
	FGameplayTag ScopeRoot;

	// How to determine a match within this scope
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relations")
	EAffiliationScopeMatchV3 Match = EAffiliationScopeMatchV3::ExactTag;

	// If true, this scope is evaluated before lower scopes (array order already implies priority)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relations")
	bool bEnabled = true;
};


UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellRelationSubsystemV3 : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	// Optional global ruleset (can be null)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relations")
	TObjectPtr<USpellRelationRulesAssetV3> RulesAsset = nullptr;
	
	// Scopes are evaluated in array order (highest priority first).
	// Recommended default: Party > Guild > Faction.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Relations")
	TArray<FAffiliationScopeV3> AffiliationScopes;


	ETargetRelationV3 ResolveRelation(const AActor* Caster, const AActor* Target) const;
	
	ETargetRelationV3 ResolveRelationDetailed(const AActor* Caster, const AActor* Target,FGameplayTag& OutReasonTag,FGameplayTag& OutMatchedA,FGameplayTag& OutMatchedB) const;


private:
	bool MatchesPair(const FGameplayTagContainer& CasterTags, const FGameplayTagContainer& TargetTags, const FSpellRelationRulePairV3& Pair) const;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	static void GatherTagsUnderRoot(const FGameplayTagContainer& Source, FGameplayTag Root, TArray<FGameplayTag>& Out);
	static bool FindExactOverlap(const TArray<FGameplayTag>& A, const TArray<FGameplayTag>& B, FGameplayTag& OutMatched);
	

	
};

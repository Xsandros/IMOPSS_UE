#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Relations/SpellRelationTypesV3.h"
#include "SpellRelationRulesAssetV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellRelationRulesAssetV3 : public UDataAsset
{
	GENERATED_BODY()
public:
	// If any (A in CasterTags && B in TargetTags) OR (B in CasterTags && A in TargetTags) => Hostile
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSpellRelationRulePairV3> HostilePairs;

	// If any pair matches => Friendly
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FSpellRelationRulePairV3> FriendlyPairs;
};

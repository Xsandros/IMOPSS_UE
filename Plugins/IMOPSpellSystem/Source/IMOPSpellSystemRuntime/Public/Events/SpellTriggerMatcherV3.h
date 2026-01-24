#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Events/SpellEventV3.h"
#include "Delivery/DeliveryEventContextV3.h"
#include "SpellTriggerMatcherV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellTriggerMatcherV3
{
	GENERATED_BODY()

	// If true, use ExactTag match. Otherwise use TagQuery.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger")
	bool bUseExactTag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger", meta=(EditCondition="bUseExactTag"))
	FGameplayTag ExactTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger", meta=(EditCondition="!bUseExactTag"))
	FGameplayTagQuery TagQuery;

	// --- Optional filters (payload-aware) ---

	// If enabled, the trigger only matches Delivery events whose payload PrimitiveId matches.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters")
	bool bFilterByPrimitiveId = false;

	// Exact primitive id match (e.g. "Delivery_E3_S0")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters", meta=(EditCondition="bFilterByPrimitiveId"))
	FName PrimitiveId = NAME_None;

	// Prefix match (e.g. "Delivery_E3")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters", meta=(EditCondition="bFilterByPrimitiveId"))
	FString PrimitiveIdPrefix;

	// If true, invert the primitive filter (exclude instead of include)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters", meta=(EditCondition="bFilterByPrimitiveId"))
	bool bInvertPrimitiveFilter = false;

	// Tag-only match (kept for existing callsites)
	bool Matches(const FGameplayTag& InTag) const
	{
		if (!InTag.IsValid())
		{
			return false;
		}

		if (bUseExactTag)
		{
			return ExactTag.IsValid() ? (InTag == ExactTag) : false;
		}

		// TagQuery matches against a container
		FGameplayTagContainer C;
		C.AddTag(InTag);
		return TagQuery.Matches(C);
	}

	// Payload-aware match (preferred)
	bool MatchesEvent(const FSpellEventV3& Ev) const
	{
		if (!Matches(Ev.EventTag))
		{
			return false;
		}

		if (!bFilterByPrimitiveId)
		{
			return true;
		}

		// Filter only applies to Delivery payloads
		const FDeliveryEventContextV3* D = Ev.Data.GetPtr<FDeliveryEventContextV3>();
		if (!D)
		{
			// If filter is enabled but payload isn't a delivery event -> no match
			return false;
		}

		const FName Pid = D->PrimitiveId;

		bool bOk = false;
		if (PrimitiveId != NAME_None)
		{
			bOk = (Pid == PrimitiveId);
		}
		else if (!PrimitiveIdPrefix.IsEmpty())
		{
			bOk = Pid.ToString().StartsWith(PrimitiveIdPrefix);
		}
		else
		{
			// Filter enabled but no criteria -> reject
			bOk = false;
		}

		if (bInvertPrimitiveFilter)
		{
			bOk = !bOk;
		}
		return bOk;
	}
};

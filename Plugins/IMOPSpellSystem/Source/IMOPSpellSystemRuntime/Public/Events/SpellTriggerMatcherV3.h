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
	
	// --- Delivery-specific int filters (string-free) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters")
	bool bFilterByEmitterIndex = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters", meta=(EditCondition="bFilterByEmitterIndex"))
	int32 EmitterIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters")
	bool bFilterBySpawnSlot = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Trigger|Filters", meta=(EditCondition="bFilterBySpawnSlot"))
	int32 SpawnSlot = 0;


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
	// NOTE: FSpellEventV3 is currently envelope-only (no typed payload stored).
	// Therefore delivery-specific filtering is performed via tags on Ev.Tags using prefix-string matching.
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

		// Filter only applies to Delivery-ish events.
		// We do NOT assume gameplay tags are registered; we match by string prefixes.
		auto HasTagWithPrefix = [&Ev](const FString& Prefix) -> bool
		{
			TArray<FGameplayTag> Tags;
			Ev.Tags.GetGameplayTagArray(Tags);

			for (const FGameplayTag& T : Tags)
			{
				const FString S = T.ToString();
				if (S.StartsWith(Prefix))
				{
					return true;
				}
			}
			return false;
		};

		// Optional: EmitterIndex filter via tags like "Delivery.EmitterIndex.3"
		if (bFilterByEmitterIndex)
		{
			const FString Prefix = FString::Printf(TEXT("Delivery.EmitterIndex.%d"), EmitterIndex);
			if (!HasTagWithPrefix(Prefix))
			{
				return false;
			}
		}

		// Optional: SpawnSlot filter via tags like "Delivery.SpawnSlot.2"
		if (bFilterBySpawnSlot)
		{
			const FString Prefix = FString::Printf(TEXT("Delivery.SpawnSlot.%d"), SpawnSlot);
			if (!HasTagWithPrefix(Prefix))
			{
				return false;
			}
		}

		// PrimitiveId filtering via:
		// - exact: "Delivery.Primitive.<PrimitiveId>"
		// - prefix: "Delivery.Primitive.<Prefix...>"
		bool bOk = false;

		if (PrimitiveId != NAME_None)
		{
			const FString Exact = FString::Printf(TEXT("Delivery.Primitive.%s"), *PrimitiveId.ToString());
			bOk = HasTagWithPrefix(Exact);
		}
		else if (!PrimitiveIdPrefix.IsEmpty())
		{
			const FString Prefix = FString::Printf(TEXT("Delivery.Primitive.%s"), *PrimitiveIdPrefix);
			bOk = HasTagWithPrefix(Prefix);
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

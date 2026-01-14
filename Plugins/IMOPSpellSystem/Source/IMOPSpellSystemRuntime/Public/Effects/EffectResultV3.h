#pragma once

#include "CoreMinimal.h"
#include "Attributes/AttributeProviderV3.h"
#include "EffectResultV3.generated.h"

UENUM(BlueprintType)
enum class EEffectOutcomeV3 : uint8
{
	Applied,
	Blocked,
	Immune,
	Resisted,
	Partial,
	Failed
};

USTRUCT(BlueprintType)
struct FEffectResultV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	EEffectOutcomeV3 Outcome = EEffectOutcomeV3::Failed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FAttributeKeyV3 Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float RawMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float FinalMagnitude = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	float MitigationFactor = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FText Reason;
};

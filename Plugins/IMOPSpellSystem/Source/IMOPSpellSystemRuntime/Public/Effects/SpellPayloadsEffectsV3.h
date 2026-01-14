#pragma once

#include "CoreMinimal.h"
#include "Effects/EffectSpecV3.h"
#include "Attributes/AttributeProviderV3.h"
#include "SpellPayloadsEffectsV3.generated.h"

USTRUCT(BlueprintType)
struct FPayload_EffectModifyAttributeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FEffectSpecV3 Effect;
};

USTRUCT(BlueprintType)
struct FPayload_ReadAttributeV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FAttributeKeyV3 Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName OutVariable = "ReadValue";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	bool bSumAcrossTargets = false;
};

USTRUCT(BlueprintType)
struct FPayload_EffectApplyForceV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FName TargetSet = "Targets";

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Payload")
	FEffectSpecV3 Effect;
};

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Effects/EffectSpecV3.h"
#include "EffectMitigationProviderV3.generated.h"

UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UEffectMitigationProviderV3 : public UInterface
{
	GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API IEffectMitigationProviderV3
{
	GENERATED_BODY()

public:
	virtual float GetMitigationFactor(const FEffectContextV3& Ctx, AActor* Target, const FEffectSpecV3& Spec) const = 0;

	virtual bool IsImmune(const FEffectContextV3& Ctx, AActor* Target, const FEffectSpecV3& Spec, FText& OutReason) const = 0;
};

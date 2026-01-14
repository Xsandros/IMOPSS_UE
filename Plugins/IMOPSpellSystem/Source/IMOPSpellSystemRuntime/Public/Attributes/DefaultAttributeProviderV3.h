#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Attributes/AttributeProviderV3.h"
#include "DefaultAttributeProviderV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDefaultAttributeProviderV3 : public UObject, public IAttributeProviderV3
{
	GENERATED_BODY()

public:
	virtual bool GetAttribute(AActor* Target, const FAttributeKeyV3& Key, float& OutValue) const override;
	virtual bool SetAttribute(AActor* Target, const FAttributeKeyV3& Key, float NewValue) const override;
	virtual bool ApplyOp(AActor* Target, const FAttributeKeyV3& Key, EAttributeOpV3 Op, float Operand, float& OutNewValue) const override;
};

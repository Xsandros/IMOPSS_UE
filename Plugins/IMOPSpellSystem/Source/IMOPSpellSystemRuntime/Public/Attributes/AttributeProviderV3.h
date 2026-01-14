#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "Attributes/AttributeProviderV3.generated.h"

USTRUCT(BlueprintType)
struct FAttributeKeyV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Attributes")
	FGameplayTag AttributeTag;
};

UENUM(BlueprintType)
enum class EAttributeOpV3 : uint8
{
	Add,
	Sub,
	Mul,
	Div,
	Override,
	Min,
	Max
};

UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UAttributeProviderV3 : public UInterface
{
	GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API IAttributeProviderV3
{
	GENERATED_BODY()

public:
	virtual bool GetAttribute(AActor* Target, const FAttributeKeyV3& Key, float& OutValue) const = 0;
	virtual bool SetAttribute(AActor* Target, const FAttributeKeyV3& Key, float NewValue) const = 0;
	virtual bool ApplyOp(AActor* Target, const FAttributeKeyV3& Key, EAttributeOpV3 Op, float Operand, float& OutNewValue) const = 0;
};

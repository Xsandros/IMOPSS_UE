#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Attributes/AttributeProviderV3.h"
#include "AttributeSubsystemV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UAttributeSubsystemV3 : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SetProvider(TScriptInterface<IAttributeProviderV3> InProvider);

	const TScriptInterface<IAttributeProviderV3>& GetProvider() const { return Provider; }

	// Convenience helpers
	bool GetAttribute(AActor* Target, const FAttributeKeyV3& Key, float& OutValue) const;
	bool ApplyOp(AActor* Target, const FAttributeKeyV3& Key, EAttributeOpV3 Op, float Operand, float& OutNewValue) const;

private:
	UPROPERTY()
	TScriptInterface<IAttributeProviderV3> Provider;
};

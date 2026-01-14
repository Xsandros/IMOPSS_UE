#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Net/UnrealNetwork.h"
#include "SpellAttributeComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellAttributeComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellAttributeComponentV3();

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	bool GetAttribute(FGameplayTag AttributeTag, float& OutValue) const;

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	void SetAttribute(FGameplayTag AttributeTag, float NewValue);

	UFUNCTION(BlueprintCallable, Category="IMOP|Attributes")
	void AddDelta(FGameplayTag AttributeTag, float Delta, float& OutNewValue);

	// Replication
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(EditAnywhere, Replicated, Category="IMOP|Attributes")
	TMap<FGameplayTag, float> Attributes;
};

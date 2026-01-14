#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EffectResistanceComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API UEffectResistanceComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="IMOP|Effects")
	float GetResistanceForTag(FGameplayTag ResistanceTag) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	TMap<FGameplayTag, float> Resistances;
};

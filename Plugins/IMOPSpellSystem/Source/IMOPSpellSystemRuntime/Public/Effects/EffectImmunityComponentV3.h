#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "EffectImmunityComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API UEffectImmunityComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="IMOP|Effects")
	bool HasImmunityTag(FGameplayTag ImmunityTag) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="IMOP|Effects")
	FGameplayTagContainer Immunities;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Status/SpellStatusTypesV3.h"
#include "Status/SpellStatusDefinitionV3.h"
#include "SpellStatusComponentV3.generated.h"

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellStatusComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellStatusComponentV3();

	UFUNCTION(BlueprintCallable, Category="IMOP|Status")
	bool HasStatus(FGameplayTag StatusTag) const;

	UFUNCTION(BlueprintCallable, Category="IMOP|Status")
	int32 GetStacks(FGameplayTag StatusTag) const;

	
	const TArray<FActiveStatusV3>& GetAll() const { return Active; }

	// internal mutation (authority only)
	bool UpsertStatus(const FActiveStatusV3& NewValue, EStatusStackPolicyV3 Policy, int32 MaxStacks);
	bool RemoveStatus(FGameplayTag StatusTag, FActiveStatusV3* OutRemoved = nullptr);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/** Authority-only: decrements timers and returns expired statuses in stable order */
	void TickStatuses(float DeltaSeconds, TArray<FActiveStatusV3>& OutExpired);
	
	UFUNCTION(BlueprintCallable, Category="IMOP|Status")
	void GetAllStatuses(TArray<FActiveStatusV3>& OutStatuses) const { OutStatuses = Active; }

	
private:
	UPROPERTY(Replicated)
	TArray<FActiveStatusV3> Active;
};

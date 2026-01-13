#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpellCoreSubsystemV3.generated.h"

class USpellActionRegistryV3;

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellCoreSubsystemV3 : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintCallable, Category = "IMOP|Spell")
	USpellActionRegistryV3* GetActionRegistry() const { return ActionRegistry; }

private:
	UPROPERTY()
	TObjectPtr<USpellActionRegistryV3> ActionRegistry;
};

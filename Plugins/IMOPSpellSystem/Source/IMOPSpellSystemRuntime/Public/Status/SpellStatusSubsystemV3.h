#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Status/SpellStatusDefinitionV3.h"
#include "Status/SpellStatusTypesV3.h"
#include "Runtime/SpellRuntimeV3.h"
#include "SpellStatusSubsystemV3.generated.h"

class USpellStatusComponentV3;

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellStatusSubsystemV3 : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;
	virtual bool IsTickable() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(USpellStatusSubsystemV3, STATGROUP_Tickables); }

	bool ApplyStatus(
		const FSpellExecContextV3& ExecCtx,
		AActor* Target,
		const FSpellStatusDefinitionV3& Def,
		const FEffectContextV3& Context,
		float DurationOverride,
		int32 StacksOverride,
		FActiveStatusV3& OutApplied);

	bool RemoveStatus(
		const FSpellExecContextV3& ExecCtx,
		AActor* Target,
		FGameplayTag StatusTag);

	bool GetStatusComponent(AActor* Target, USpellStatusComponentV3*& OutComp) const;

private:
	void HandleExpiry(AActor* Target, const FActiveStatusV3& Status);
};

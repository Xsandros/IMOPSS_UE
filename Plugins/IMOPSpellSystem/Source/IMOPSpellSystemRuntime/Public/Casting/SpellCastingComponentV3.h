#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Spec/SpellSpecV3.h"
#include "SpellCastingComponentV3.generated.h"

class USpellRuntimeV3;

/**
 * Simple MP-safe casting helper:
 * - stores a spell loadout (DataAssets)
 * - supports casting by index or direct spec
 * - routes client input to server via RPC
 */
UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellCastingComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellCastingComponentV3();

	/** A simple list of SpellSpec assets you can assign in the editor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Spell|Casting")
	TArray<TObjectPtr<USpellSpecV3>> Loadout;

	/** If true, runtimes created by this component are kept alive in ActiveRuntimes until StopAll() (useful for debugging). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Spell|Casting")
	bool bKeepRuntimesAliveForDebug = true;

	/** Base seed used for deterministic RNG seeding (server-side). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="IMOP|Spell|Casting")
	int32 BaseSeed = 1337;

	/** Cast a spell by loadout index (0-based). Client-safe: routes to server. */
	UFUNCTION(BlueprintCallable, Category="IMOP|Spell|Casting")
	bool CastLoadoutIndex(int32 Index);

	/** Cast a specific spell spec (DataAsset). Client-safe: routes to server. */
	UFUNCTION(BlueprintCallable, Category="IMOP|Spell|Casting")
	bool CastSpell(USpellSpecV3* Spec);

	/** Stop and clear all active runtimes created by this component. */
	UFUNCTION(BlueprintCallable, Category="IMOP|Spell|Casting")
	void StopAll();

	/** Returns the last runtime created (if any). Handy for debugging. */
	UFUNCTION(BlueprintCallable, Category="IMOP|Spell|Casting")
	USpellRuntimeV3* GetLastRuntime() const { return LastRuntime; }

protected:
	virtual void BeginPlay() override;

private:
	// ===== RPC =====
	UFUNCTION(Server, Reliable)
	void ServerCastSpell(USpellSpecV3* Spec);

	void CastSpell_Internal(USpellSpecV3* Spec);

	// ===== State =====
	UPROPERTY()
	TArray<TObjectPtr<USpellRuntimeV3>> ActiveRuntimes;

	UPROPERTY()
	TObjectPtr<USpellRuntimeV3> LastRuntime = nullptr;

	int32 CastCounter = 0;
};

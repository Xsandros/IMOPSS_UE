#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputAction.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "SpellInputRouterComponentV3.generated.h"

class UEnhancedInputComponent;
class UInputAction;
class USpellInputBindingsAssetV3;
class USpellRuntimeV3;

UCLASS(ClassGroup=(IMOP), meta=(BlueprintSpawnableComponent))
class IMOPSPELLSYSTEMRUNTIME_API USpellInputRouterComponentV3 : public UActorComponent
{
	GENERATED_BODY()

public:
	USpellInputRouterComponentV3();

	// The binding table (DataAsset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	TObjectPtr<const USpellInputBindingsAssetV3> BindingsAsset = nullptr;

	UPROPERTY(Transient)
	bool bMappingApplied = false;

	UPROPERTY(Transient)
	bool bInputBound = false;

	UPROPERTY(Transient)
	TArray<uint32> BoundHandles; // optional, falls du später Unbind sauber machen willst

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// NEW: explicit bind step (idempotent)
	void BindInputIfPossible();
	void UnbindInputIfNeeded();

	UEnhancedInputComponent* GetEnhancedInputComponent() const;

	void AddMappingContextIfPossible();
	void RemoveMappingContextIfPossible();

	// Dispatcher
	void OnAnyInputAction(const FInputActionInstance& Instance);

	void ExecuteSlotByIndex(int32 SlotIndex);


private:
	// Action* -> SlotIndex (deterministic, stable)
	TMap<TObjectPtr<const UInputAction>, int32> ActionToIndex;
	

	// Tracks whether we added the mapping context
	bool bAddedContext = false;
	
	// NEW: store binding handles so we can unbind (prevents stacking)
	UPROPERTY(Transient)
	TArray<int32> BindingHandles;
	
	// pro Slot eine aktive Runtime (UPROPERTY, damit GC-safe)
	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<USpellRuntimeV3>> ActiveRuntimeBySlot;
};

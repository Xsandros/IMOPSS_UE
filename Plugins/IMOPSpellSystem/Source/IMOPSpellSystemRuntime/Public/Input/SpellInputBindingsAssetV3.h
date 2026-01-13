#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h" // optional, wenn du types davon nutzt
#include "EnhancedInputComponent.h" // bringt ETriggerEvent

#include "SpellInputBindingsAssetV3.generated.h"

class USpellSpecV3;

UENUM(BlueprintType)
enum class ESpellInputBindingModeV3 : uint8
{
	EmitEventOnly UMETA(DisplayName="Emit Event"),
	StartSpellOnly UMETA(DisplayName="Start Spell"),
	EmitEventAndStartSpell UMETA(DisplayName="Emit Event + Start Spell")
};


USTRUCT(BlueprintType)
struct FSpellInputBindingSlotV3
{
	GENERATED_BODY()

	// Which Enhanced Input action triggers this slot
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	TObjectPtr<const UInputAction> Action = nullptr;

	// Which spell to cast
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	TObjectPtr<USpellSpecV3> Spell = nullptr;

	// Which trigger event to bind
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	ETriggerEvent TriggerEvent = ETriggerEvent::Started;

	// FINAL: Was soll passieren?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Input")
	ESpellInputBindingModeV3 Mode = ESpellInputBindingModeV3::EmitEventOnly;

	// FINAL: Welcher Tag wird bei Input gesendet?
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Input", meta=(EditCondition="Mode!=ESpellInputBindingModeV3::StartSpellOnly"))
	FGameplayTag EmitEventTag;
	
	// Optional deterministic seed override for this slot
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	bool bOverrideSeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input", meta=(EditCondition="bOverrideSeed"))
	int32 SeedOverride = 1337;
};

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellInputBindingsAssetV3 : public UDataAsset
{
	GENERATED_BODY()
public:
	// Mapping context to add at runtime
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	TObjectPtr<const UInputMappingContext> MappingContext = nullptr;

	// Context priority
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	int32 Priority = 0;

	// Slots
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Spell|Input")
	TArray<FSpellInputBindingSlotV3> Bindings;
};

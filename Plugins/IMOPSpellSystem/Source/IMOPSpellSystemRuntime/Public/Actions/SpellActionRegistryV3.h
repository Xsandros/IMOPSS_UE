#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "SpellActionRegistryV3.generated.h"

USTRUCT()
struct FRegisteredActionBindingV3
{
	GENERATED_BODY()

	UPROPERTY() FGameplayTag ActionTag;
	UPROPERTY() TSubclassOf<class USpellActionExecutorV3> ExecutorClass;

	// Raw pointer (not serialized). OK for runtime contract registry.
	const UScriptStruct* PayloadStruct = nullptr;
};

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellActionRegistryV3 : public UObject
{
	GENERATED_BODY()
public:
	bool RegisterBinding(
		const FGameplayTag& ActionTag,
		TSubclassOf<class USpellActionExecutorV3> ExecutorClass,
		const UScriptStruct* PayloadStruct,
		bool bAllowOverride,
		FText* OutError = nullptr
	);

	const FRegisteredActionBindingV3* FindBinding(const FGameplayTag& ActionTag) const;

	void GetAllBindings(TArray<FRegisteredActionBindingV3>& Out) const;

private:
	TMap<FGameplayTag, FRegisteredActionBindingV3> Bindings;
};

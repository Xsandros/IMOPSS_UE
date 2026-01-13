#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class USpellActionRegistryV3;
class USpellActionExecutorV3;
class UScriptStruct;

struct FActionBindingV3
{
	FGameplayTag ActionTag;
	TSubclassOf<USpellActionExecutorV3> ExecutorClass;
	const UScriptStruct* PayloadStruct = nullptr;
};

class IMOPSPELLSYSTEMRUNTIME_API FActionTagContractsV3
{
public:
	static void RegisterDefaults(USpellActionRegistryV3& Registry);

	static bool ValidateBinding(
		const FGameplayTag& ActionTag,
		TSubclassOf<USpellActionExecutorV3> ExecutorClass,
		const UScriptStruct* PayloadStruct,
		FText& OutError);

	static const UScriptStruct* ExpectedPayloadStruct(const FGameplayTag& ActionTag);

	static void DumpDefaultsToLog();
};

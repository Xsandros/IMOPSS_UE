#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SpellDebugBPLibraryV3.generated.h"

struct FSpellEventV3;

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellDebugBPLibraryV3 : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="IMOP|Spell|Debug")
	static FString DescribeSpellEvent(const FSpellEventV3& Ev);
};

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Events/SpellEventV3.h"
#include "Debug/SpellTraceSubsystemV3.h"
#include "SpellDebugBPLibraryV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellDebugBPLibraryV3 : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="IMOP|Debug", meta=(WorldContext="WorldContextObject"))
	static USpellTraceSubsystemV3* GetTraceSubsystem(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	static FString FormatEventShort(const FSpellEventV3& Ev);

	UFUNCTION(BlueprintCallable, Category="IMOP|Debug")
	static FString FormatRowShort(const FSpellTraceRowV3& Row);
};

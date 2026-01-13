#pragma once

#include "CoreMinimal.h"
#include "Spec/SpellActionV3.h"

class USpellRuntimeV3;

class IMOPSPELLSYSTEMRUNTIME_API FSpellVMV3
{
public:
    static void ExecuteActions(USpellRuntimeV3& Runtime, const TArray<FSpellActionV3>& Actions);
};

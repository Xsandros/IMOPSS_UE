#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingTypesV3.h"

struct FSpellExecContextV3;

struct IMOPSPELLSYSTEMRUNTIME_API FTargetingSelectionV3
{
	static void Select(
		const FSpellExecContextV3& Ctx,
		const FVector& Origin,
		const FTargetSelectV3& SelectSpec,
		const TArray<FTargetRefV3>& Candidates,
		TArray<FTargetRefV3>& OutSelected);
};

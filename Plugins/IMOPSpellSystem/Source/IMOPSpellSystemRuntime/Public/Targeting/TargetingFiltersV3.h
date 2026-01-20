#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingTypesV3.h"

struct FSpellExecContextV3;
class ITargetingGameHooksV3;

struct IMOPSPELLSYSTEMRUNTIME_API FTargetingFiltersV3
{
	static void ApplyFilters(
		const FSpellExecContextV3& Ctx,
		const ITargetingGameHooksV3& Hooks,
		const FVector& Origin,
		const TArray<FTargetFilterV3>& Filters,
		TArray<FTargetRefV3>& InOutCandidates);
};

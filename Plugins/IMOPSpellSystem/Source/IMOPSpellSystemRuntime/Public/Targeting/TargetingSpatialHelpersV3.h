#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingTypesV3.h"

struct FSpellExecContextV3;

struct IMOPSPELLSYSTEMRUNTIME_API FTargetingSpatialHelpersV3
{
    static FVector ResolveOriginLocation(const FSpellExecContextV3& Ctx, const FTargetOriginV3& Origin);

    static void SortByDistanceTo(TArray<FTargetRefV3>& InOut, const FVector& OriginLoc, bool bAscending);

    static FVector ComputeCenter(const TArray<FTargetRefV3>& Targets);
};

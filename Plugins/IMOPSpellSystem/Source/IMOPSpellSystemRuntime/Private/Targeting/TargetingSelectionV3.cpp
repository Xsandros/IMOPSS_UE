#include "Targeting/TargetingSelectionV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Targeting/TargetingSpatialHelpersV3.h"

static void MakeUniqueActors(TArray<FTargetRefV3>& InOut)
{
    TSet<TWeakObjectPtr<AActor>> Seen;
    InOut.RemoveAll([&](const FTargetRefV3& R)
    {
        if (!R.IsValid()) return true;
        if (Seen.Contains(R.Actor)) return true;
        Seen.Add(R.Actor);
        return false;
    });
}

void FTargetingSelectionV3::Select(
    const FSpellExecContextV3& Ctx,
    const FVector& Origin,
    const FTargetSelectV3& SelectSpec,
    const TArray<FTargetRefV3>& Candidates,
    TArray<FTargetRefV3>& OutSelected)
{
    OutSelected.Reset();

    TArray<FTargetRefV3> Work = Candidates;
    Work.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });

    if (SelectSpec.bUnique)
    {
        MakeUniqueActors(Work);
    }

    const int32 WantCount = (SelectSpec.Count <= 0) ? Work.Num() : FMath::Min(SelectSpec.Count, Work.Num());

    switch (SelectSpec.Kind)
    {
    case ETargetSelectKindV3::All:
        OutSelected = Work;
        return;

    case ETargetSelectKindV3::Nearest:
        FTargetingSpatialHelpersV3::SortByDistanceTo(Work, Origin, /*bAscending*/ true);
        OutSelected.Append(Work.GetData(), WantCount);
        return;

    case ETargetSelectKindV3::Farthest:
        FTargetingSpatialHelpersV3::SortByDistanceTo(Work, Origin, /*bAscending*/ false);
        OutSelected.Append(Work.GetData(), WantCount);
        return;

    case ETargetSelectKindV3::Random:
        {
            FRandomStream& Rng = Ctx.GetRng();

            // deterministisch: Fisher-Yates shuffle
            for (int32 i = Work.Num() - 1; i > 0; --i)
            {
                const int32 j = Rng.RandRange(0, i);
                Work.Swap(i, j);
            }

            OutSelected.Append(Work.GetData(), WantCount);
            return;
        }

    default:
        OutSelected = Work;
        return;
    }
}

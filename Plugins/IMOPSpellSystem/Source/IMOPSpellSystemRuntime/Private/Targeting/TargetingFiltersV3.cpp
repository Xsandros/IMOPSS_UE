#include "Targeting/TargetingFiltersV3.h"

#include "Actions/SpellActionExecutorV3.h"
#include "Targeting/TargetingGameHooksV3.h"

static bool PassesOne(
    const FSpellExecContextV3& Ctx,
    const ITargetingGameHooksV3& Hooks,
    const FVector& Origin,
    const FTargetFilterV3& Filt,
    const FTargetRefV3& Cand)
{
    const AActor* A = Cand.Actor.Get();
    if (!A) return false;

    switch (Filt.Kind)
    {
    case ETargetFilterKindV3::Relation:
        {
            const ETargetRelationV3 Rel = Hooks.GetRelation(Ctx, A);
            if (Filt.Relation == ETargetRelationV3::Any) return true;
            return Rel == Filt.Relation;
        }

    case ETargetFilterKindV3::HasTag:
        return Hooks.HasGameplayTag(A, Filt.Tag);

    case ETargetFilterKindV3::HasStatus:
        return Hooks.HasStatusTag(A, Filt.Tag);

    case ETargetFilterKindV3::LineOfSight:
        return Hooks.HasLineOfSight(Ctx, Origin, A);

    case ETargetFilterKindV3::DistanceRange:
        {
            const float D = FVector::Dist(A->GetActorLocation(), Origin);
            const bool bMinOk = (Filt.MinDistance <= 0.f) || (D >= Filt.MinDistance);
            const bool bMaxOk = (Filt.MaxDistance <= 0.f) || (D <= Filt.MaxDistance);
            return bMinOk && bMaxOk;
        }

    default:
        return true;
    }
}

void FTargetingFiltersV3::ApplyFilters(
    const FSpellExecContextV3& Ctx,
    const ITargetingGameHooksV3& Hooks,
    const FVector& Origin,
    const TArray<FTargetFilterV3>& Filters,
    TArray<FTargetRefV3>& InOutCandidates)
{
    if (Filters.Num() == 0)
    {
        // trotzdem invalid rauswerfen
        InOutCandidates.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });
        return;
    }

    // remove invalid first
    InOutCandidates.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });

    for (const FTargetFilterV3& Filt : Filters)
    {
        InOutCandidates.RemoveAll([&](const FTargetRefV3& Cand)
        {
            const bool bMatch = PassesOne(Ctx, Hooks, Origin, Filt, Cand);
            // Keep = entferne wenn NICHT match
            // Drop = entferne wenn match
            return (Filt.Op == ETargetFilterOpV3::Keep) ? (!bMatch) : (bMatch);
        });
    }
}

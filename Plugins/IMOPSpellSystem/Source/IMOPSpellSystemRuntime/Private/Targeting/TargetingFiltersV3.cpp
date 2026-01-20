#include "Targeting/TargetingFiltersV3.h"
#include "Targeting/TargetingGameHooksV3.h"

#include "Attributes/SpellAttributeComponentV3.h"
#include "Status/SpellStatusComponentV3.h"

static bool HasAnyTagViaHooks(const ITargetingGameHooksV3& Hooks, const AActor* A, const FGameplayTagContainer& Tags)
{
    if (!A) return false;
    for (const FGameplayTag& T : Tags)
    {
        if (T.IsValid() && Hooks.HasGameplayTag(A, T))
        {
            return true;
        }
    }
    return false;
}

static bool HasAllTagsViaHooks(const ITargetingGameHooksV3& Hooks, const AActor* A, const FGameplayTagContainer& Tags)
{
    if (!A) return false;
    for (const FGameplayTag& T : Tags)
    {
        if (T.IsValid() && !Hooks.HasGameplayTag(A, T))
        {
            return false;
        }
    }
    return true;
}

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

        // --- Legacy single tag ---
        case ETargetFilterKindV3::HasTag:
            return Filt.Tag.IsValid() ? Hooks.HasGameplayTag(A, Filt.Tag) : true;

        // --- Tag containers (final) ---
        case ETargetFilterKindV3::HasAnyTags:
            return (Filt.Tags.Num() == 0) ? true : HasAnyTagViaHooks(Hooks, A, Filt.Tags);

        case ETargetFilterKindV3::HasAllTags:
            return (Filt.Tags.Num() == 0) ? true : HasAllTagsViaHooks(Hooks, A, Filt.Tags);

        case ETargetFilterKindV3::HasNoneTags:
            return (Filt.Tags.Num() == 0) ? true : !HasAnyTagViaHooks(Hooks, A, Filt.Tags);

        // --- Status ---
        case ETargetFilterKindV3::HasStatus:
            return Filt.Tag.IsValid() ? Hooks.HasStatusTag(A, Filt.Tag) : true;

        // --- LoS ---
        case ETargetFilterKindV3::LineOfSight:
            return Hooks.HasLineOfSight(Ctx, Origin, A);

        // --- Distance ---
        case ETargetFilterKindV3::DistanceRange:
        {
            const float D = FVector::Dist(A->GetActorLocation(), Origin);
            const bool bMinOk = (Filt.MinDistance <= 0.f) || (D >= Filt.MinDistance);
            const bool bMaxOk = (Filt.MaxDistance <= 0.f) || (D <= Filt.MaxDistance);
            return bMinOk && bMaxOk;
        }

        // --- Components (final) ---
        case ETargetFilterKindV3::HasComponent:
        {
            if (!*Filt.ComponentClass) return true; // nothing requested
            return A->FindComponentByClass(Filt.ComponentClass) != nullptr;
        }

        case ETargetFilterKindV3::HasAttributeComponent:
            return A->FindComponentByClass<USpellAttributeComponentV3>() != nullptr;

        case ETargetFilterKindV3::HasStatusComponent:
            return A->FindComponentByClass<USpellStatusComponentV3>() != nullptr;

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
    // Remove invalid first (always)
    InOutCandidates.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });

    if (Filters.Num() == 0)
    {
        return;
    }

    for (const FTargetFilterV3& Filt : Filters)
    {
        InOutCandidates.RemoveAll([&](const FTargetRefV3& Cand)
        {
            const bool bMatch = PassesOne(Ctx, Hooks, Origin, Filt, Cand);

            // Keep = remove if NOT match
            // Drop = remove if match
            return (Filt.Op == ETargetFilterOpV3::Keep) ? (!bMatch) : (bMatch);
        });
    }
}

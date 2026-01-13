#include "Targeting/TargetingGameHooks_DefaultV3.h"

#include "Actions/SpellActionExecutorV3.h" // FSpellExecContextV3
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "Runtime/SpellRuntimeV3.h"   // <- damit USpellRuntimeV3 vollständig ist

#include "Engine/World.h"

ETargetRelationV3 UTargetingGameHooks_DefaultV3::GetRelation(const FSpellExecContextV3& Ctx, const AActor* Candidate) const
{
    const AActor* Caster = Ctx.Caster;
    if (Candidate && Caster && Candidate == Caster)
    {
        return ETargetRelationV3::Self;
    }
    return ETargetRelationV3::Any;
}

bool UTargetingGameHooks_DefaultV3::HasGameplayTag(const AActor* Candidate, FGameplayTag Tag) const
{
    if (!Candidate || !Tag.IsValid()) return false;

    if (const IGameplayTagAssetInterface* TagIfc = Cast<IGameplayTagAssetInterface>(Candidate))
    {
        FGameplayTagContainer Tags;
        TagIfc->GetOwnedGameplayTags(Tags);
        return Tags.HasTag(Tag);
    }
    return false;
}

bool UTargetingGameHooks_DefaultV3::HasStatusTag(const AActor* Candidate, FGameplayTag StatusTag) const
{
    // Phase 2: noch kein Status-System -> immer false
    return false;
}

bool UTargetingGameHooks_DefaultV3::HasLineOfSight(const FSpellExecContextV3& Ctx, const FVector& Origin, const AActor* Candidate) const
{
    if (!Candidate) return false;

    UWorld* World = nullptr;

    if (USpellRuntimeV3* RT = Ctx.Runtime.Get())
    {
        World = RT->GetWorld();
    }
    if (!World && Ctx.WorldContext)
    {
        World = Ctx.WorldContext->GetWorld();
    }

    if (!World)
    {
        return false; // korrekt & final
    }


    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(IMOP_TargetingLOS), /*bTraceComplex*/ false);

    if (Ctx.Caster)
    {
        Params.AddIgnoredActor(Ctx.Caster);
    }

    const FVector End = Candidate->GetActorLocation();
    const bool bHit = World->LineTraceSingleByChannel(Hit, Origin, End, LineTraceChannel, Params);

    // Wenn wir nichts blockendes treffen -> LOS ok
    if (!bHit) return true;

    // Wenn das erste Hit-Actor der Candidate ist -> auch ok
    return (Hit.GetActor() == Candidate);
}

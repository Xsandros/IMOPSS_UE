#include "Targeting/TargetingGameHooks_DefaultV3.h"

#include "Actions/SpellActionExecutorV3.h" // FSpellExecContextV3
#include "GameFramework/Actor.h"
#include "GameplayTagAssetInterface.h"
#include "Runtime/SpellRuntimeV3.h"   // <- damit USpellRuntimeV3 vollständig ist
#include "Relations/SpellRelationSubsystemV3.h"
#include "Relations/SpellRelationSubsystemV3.h"
#include "Relations/SpellRelationEventDataV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "StructUtils/InstancedStruct.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Core/SpellExecContextHelpersV3.h" // for IMOP_GetGIFromExecCtx
#include "Engine/World.h"

ETargetRelationV3 UTargetingGameHooks_DefaultV3::GetRelation(const FSpellExecContextV3& Ctx, const AActor* Candidate) const
{
    const AActor* Caster = Ctx.Caster;
    if (Candidate && Caster && Candidate == Caster)
    {
        return ETargetRelationV3::Self;
    }

    // Find World
    UWorld* World = nullptr;
    if (USpellRuntimeV3* RT = Ctx.Runtime.Get())
    {
        World = RT->GetWorld();
    }
    if (!World && Ctx.WorldContext)
    {
        World = Ctx.WorldContext->GetWorld();
    }

    // Fallback if no world
    if (!World)
    {
        return ETargetRelationV3::Any;
    }

    ETargetRelationV3 Result = ETargetRelationV3::Any;

    FGameplayTag ReasonTag;
    FGameplayTag MatchedA;
    FGameplayTag MatchedB;

    if (USpellRelationSubsystemV3* Rel = World->GetSubsystem<USpellRelationSubsystemV3>())
    {
        Result = Rel->ResolveRelationDetailed(Caster, Candidate, ReasonTag, MatchedA, MatchedB);
    }
    else
    {
        Result = ETargetRelationV3::Any;
    }

    // Emit debug event (optional)
    if (bEmitRelationDebugEvents)
    {
        if (UGameInstance* GI = IMOP_GetGIFromExecCtx(Ctx))
        {
            if (USpellEventBusSubsystemV3* Bus = GI->GetSubsystem<USpellEventBusSubsystemV3>())
            {
                const auto& Tags = FIMOPSpellGameplayTagsV3::Get();

                FSpellEventV3 Ev;
                Ev.EventTag = Tags.Event_Relation_Resolved;

                Ev.Caster = const_cast<AActor*>(Caster);
                Ev.Sender = Ctx.Runtime.Get(); // runtime as sender

                Ev.FrameNumber = GFrameNumber;
                Ev.TimeSeconds = World->GetTimeSeconds();
                Ev.RuntimeGuid = (Ctx.Runtime ? Ctx.Runtime->GetRuntimeGuid() : FGuid());

                FSpellEventData_RelationResolvedV3 Data;
                Data.Target = const_cast<AActor*>(Candidate);
                Data.Relation = Result;
                Data.ReasonTag = ReasonTag;
                Data.MatchedA = MatchedA;
                Data.MatchedB = MatchedB;

                Ev.Data = FInstancedStruct::Make<FSpellEventData_RelationResolvedV3>(Data);


                Bus->Emit(Ev);
            }
        }
    }

    return Result;
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

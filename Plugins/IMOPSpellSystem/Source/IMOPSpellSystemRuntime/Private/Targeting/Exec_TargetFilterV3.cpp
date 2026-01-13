#include "Targeting/Exec_TargetFilterV3.h"

#include "Targeting/SpellPayloadsTargetingV3.h"
#include "Targeting/TargetingSpatialHelpersV3.h"
#include "Targeting/TargetingFiltersV3.h"

#include "Stores/SpellTargetStoreV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Targeting/TargetingGameHooksV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecTargetFilterV3, Log, All);

static UWorld* GetWorldFromExecCtx(const FSpellExecContextV3& Ctx)
{
    if (Ctx.Runtime)
    {
        return Ctx.Runtime->GetWorld();
    }
    if (Ctx.Caster)
    {
        return Ctx.Caster->GetWorld();
    }
    if (Ctx.WorldContext)
    {
        if (GEngine)
        {
            return GEngine->GetWorldFromContextObject(Ctx.WorldContext, EGetWorldErrorMode::ReturnNull);
        }
    }
    return nullptr;
}

const UScriptStruct* UExec_TargetFilterV3::GetPayloadStruct() const
{
    return FPayload_TargetFilterV3::StaticStruct();
}

void UExec_TargetFilterV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_TargetFilterV3* P = static_cast<const FPayload_TargetFilterV3*>(PayloadData);
    if (!P)
    {
        UE_LOG(LogIMOPExecTargetFilterV3, Error, TEXT("TargetFilter: Payload is null."));
        return;
    }
    if (!Ctx.TargetStore)
    {
        UE_LOG(LogIMOPExecTargetFilterV3, Error, TEXT("TargetFilter: TargetStore missing."));
        return;
    }

    const FTargetSetV3* InSet = Ctx.TargetStore->Find(P->InTargetSet);
    if (!InSet)
    {
        UE_LOG(LogIMOPExecTargetFilterV3, Warning, TEXT("TargetFilter: InTargetSet not found: %s"), *P->InTargetSet.ToString());
        return;
    }

    UWorld* World = GetWorldFromExecCtx(Ctx);
    if (!World)
    {
        UE_LOG(LogIMOPExecTargetFilterV3, Error, TEXT("TargetFilter: World missing."));
        return;
    }

    const FVector OriginLoc = FTargetingSpatialHelpersV3::ResolveOriginLocation(Ctx, P->Origin);

    // Copy candidates from set
    TArray<FTargetRefV3> Candidates = InSet->Targets;
    Candidates.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });

    // Get hooks
    UTargetingSubsystemV3* Sub = World ? World->GetSubsystem<UTargetingSubsystemV3>() : nullptr;
    UObject* HooksObj = Sub ? Sub->GetGameHooksObject() : nullptr;

    ITargetingGameHooksV3* Hooks = HooksObj ? Cast<ITargetingGameHooksV3>(HooksObj) : nullptr;
    if (!Hooks)
    {
        UE_LOG(LogIMOPExecTargetFilterV3, Error, TEXT("TargetFilter: Missing GameHooks (ITargetingGameHooksV3)."));
        return;
    }

    FTargetingFiltersV3::ApplyFilters(Ctx, *Hooks, OriginLoc, P->Filters, Candidates);



    // Write output set
    FTargetSetV3 OutSet;
    OutSet.Targets = Candidates;
    OutSet.RemoveInvalid();

    Ctx.TargetStore->Set(P->OutTargetSet, OutSet);

    UE_LOG(LogIMOPExecTargetFilterV3, Log, TEXT("TargetFilter: In=%s Out=%s Result=%d"),
        *P->InTargetSet.ToString(), *P->OutTargetSet.ToString(), OutSet.Targets.Num());
}

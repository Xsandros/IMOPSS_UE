#include "Targeting/Exec_TargetSelectV3.h"

#include "Targeting/SpellPayloadsTargetingV3.h"
#include "Targeting/TargetingSpatialHelpersV3.h"
#include "Targeting/TargetingSelectionV3.h"

#include "Stores/SpellTargetStoreV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecTargetSelectV3, Log, All);

static UWorld* GetWorldFromExecCtx(const FSpellExecContextV3& Ctx)
{
    return Ctx.WorldContext ? Ctx.WorldContext->GetWorld() : nullptr;
}

static UGameInstance* GetGIFromExecCtx(const FSpellExecContextV3& Ctx)
{
    if (UWorld* W = GetWorldFromExecCtx(Ctx))
    {
        return W->GetGameInstance();
    }
    return nullptr;
}

const UScriptStruct* UExec_TargetSelectV3::GetPayloadStruct() const
{
    return FPayload_TargetSelectV3::StaticStruct();
}

void UExec_TargetSelectV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_TargetSelectV3* P = static_cast<const FPayload_TargetSelectV3*>(PayloadData);
    if (!P)
    {
        UE_LOG(LogIMOPExecTargetSelectV3, Error, TEXT("TargetSelect: Payload is null."));
        return;
    }
    if (!Ctx.TargetStore)
    {
        UE_LOG(LogIMOPExecTargetSelectV3, Error, TEXT("TargetSelect: TargetStore missing."));
        return;
    }

    const FTargetSetV3* InSet = Ctx.TargetStore->Find(P->InTargetSet);
    if (!InSet)
    {
        UE_LOG(LogIMOPExecTargetSelectV3, Warning, TEXT("TargetSelect: InTargetSet not found: %s"), *P->InTargetSet.ToString());
        return;
    }

    UWorld* World = GetWorldFromExecCtx(Ctx);
    if (!World)
    {
        UE_LOG(LogIMOPExecTargetSelectV3, Error, TEXT("TargetSelect: World missing."));
        return;
    }

    const FVector OriginLoc = FTargetingSpatialHelpersV3::ResolveOriginLocation(Ctx, P->Origin);

    // Candidates
    TArray<FTargetRefV3> Candidates = InSet->Targets;
    Candidates.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); });

    // Select
    TArray<FTargetRefV3> Selected;
    FTargetingSelectionV3::Select(Ctx, OriginLoc, P->Selection, Candidates, Selected);

    FTargetSetV3 OutSet;
    OutSet.Targets = Selected;
    OutSet.RemoveInvalid();

    Ctx.TargetStore->Set(P->OutTargetSet, OutSet);

    UE_LOG(LogIMOPExecTargetSelectV3, Log, TEXT("TargetSelect: In=%s Out=%s Selected=%d"),
        *P->InTargetSet.ToString(), *P->OutTargetSet.ToString(), OutSet.Targets.Num());
}

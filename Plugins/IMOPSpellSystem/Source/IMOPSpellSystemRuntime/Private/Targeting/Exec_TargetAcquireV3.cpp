#include "Targeting/Exec_TargetAcquireV3.h"

#include "Targeting/SpellPayloadsTargetingV3.h"
#include "Targeting/TargetingSubsystemV3.h"

#include "Stores/SpellTargetStoreV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecTargetAcquireV3, Log, All);

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

const UScriptStruct* UExec_TargetAcquireV3::GetPayloadStruct() const
{
    return FPayload_TargetAcquireV3::StaticStruct();
}

void UExec_TargetAcquireV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_TargetAcquireV3* P = static_cast<const FPayload_TargetAcquireV3*>(PayloadData);
    if (!P)
    {
        UE_LOG(LogIMOPExecTargetAcquireV3, Error, TEXT("TargetAcquire: Payload is null."));
        return;
    }
    if (!Ctx.TargetStore)
    {
        UE_LOG(LogIMOPExecTargetAcquireV3, Error, TEXT("TargetAcquire: TargetStore missing."));
        return;
    }

    UWorld* World = GetWorldFromExecCtx(Ctx);
    if (!World)
    {
        UE_LOG(LogIMOPExecTargetAcquireV3, Error, TEXT("TargetAcquire: World missing."));
        return;
    }

    UTargetingSubsystemV3* Sub = World->GetSubsystem<UTargetingSubsystemV3>();
    if (!Sub)
    {
        UE_LOG(LogIMOPExecTargetAcquireV3, Error, TEXT("TargetAcquire: TargetingSubsystemV3 missing."));
        return;
    }

    FTargetAcquireResponseV3 Resp;
    const bool bOk = Sub->AcquireTargets(Ctx, P->Request, Resp);

    if (bOk)
    {
        // Store Selected into OutTargetSet
        FTargetSetV3 OutSet;
        OutSet.Targets = Resp.Selected;
        OutSet.RemoveInvalid();

        Ctx.TargetStore->Set(P->OutTargetSet, OutSet);

        UE_LOG(LogIMOPExecTargetAcquireV3, Log, TEXT("TargetAcquire: OutSet=%s Selected=%d Candidates=%d"),
            *P->OutTargetSet.ToString(),
            Resp.Selected.Num(),
            Resp.Candidates.Num());
    }
    else
    {
        UE_LOG(LogIMOPExecTargetAcquireV3, Warning, TEXT("TargetAcquire FAILED: %s"),
            *Resp.Error.ToString());
    }

    // Optional: Event emission (nur wenn du diese Tags + EventBus API schon hast)
    // -> Wenn du mir kurz sagst wie dein EventBus Emit heißt, passe ich das 1:1 an.
}

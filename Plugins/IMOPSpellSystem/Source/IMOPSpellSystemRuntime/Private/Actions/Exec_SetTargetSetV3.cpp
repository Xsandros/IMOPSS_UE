#include "Actions/Exec_SetTargetSetV3.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Targeting/TargetingTypesV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecSetTargetSetV3, Log, All);

const UScriptStruct* UExec_SetTargetSetV3::GetPayloadStruct() const
{
    return FPayload_SetTargetSetV3::StaticStruct();
}

void UExec_SetTargetSetV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_SetTargetSetV3* P = static_cast<const FPayload_SetTargetSetV3*>(PayloadData);
    if (!P || P->TargetSetKey == NAME_None)
    {
        UE_LOG(LogIMOPExecSetTargetSetV3, Warning, TEXT("SetTargetSet: invalid payload."));
        return;
    }
    if (!Ctx.TargetStore)
    {
        UE_LOG(LogIMOPExecSetTargetSetV3, Error, TEXT("SetTargetSet: missing TargetStore."));
        return;
    }

    // Final-form store uses FTargetSetV3
    FTargetSetV3 Set;
    Set.Targets.Reserve(P->Actors.Num());
    for (AActor* A : P->Actors)
    {
        if (!IsValid(A)) continue;
        FTargetRefV3 Ref;
        Ref.Actor = A;
        Set.AddUnique(Ref);
    }

    Ctx.TargetStore->Set(P->TargetSetKey, Set);
}

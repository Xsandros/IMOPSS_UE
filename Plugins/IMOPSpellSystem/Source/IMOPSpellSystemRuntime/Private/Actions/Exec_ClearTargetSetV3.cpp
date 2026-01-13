#include "Actions/Exec_ClearTargetSetV3.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "Stores/SpellTargetStoreV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecClearTargetSetV3, Log, All);

const UScriptStruct* UExec_ClearTargetSetV3::GetPayloadStruct() const
{
    return FPayload_ClearTargetSetV3::StaticStruct();
}

void UExec_ClearTargetSetV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_ClearTargetSetV3* P = static_cast<const FPayload_ClearTargetSetV3*>(PayloadData);
    if (!P || P->TargetSetKey == NAME_None)
    {
        UE_LOG(LogIMOPExecClearTargetSetV3, Warning, TEXT("ClearTargetSet: invalid payload."));
        return;
    }
    if (!Ctx.TargetStore)
    {
        UE_LOG(LogIMOPExecClearTargetSetV3, Error, TEXT("ClearTargetSet: missing TargetStore."));
        return;
    }

    // Final-form store uses Clear()
    Ctx.TargetStore->Clear(P->TargetSetKey);
}

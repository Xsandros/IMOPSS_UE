#include "Actions/Exec_SetVariableV3.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "Stores/SpellVariableStoreV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecSetVarV3, Log, All);

const UScriptStruct* UExec_SetVariableV3::GetPayloadStruct() const
{
    return FPayload_SetVariableV3::StaticStruct();
}

void UExec_SetVariableV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_SetVariableV3* P = static_cast<const FPayload_SetVariableV3*>(PayloadData);
    if (!P || P->Key == NAME_None)
    {
        UE_LOG(LogIMOPExecSetVarV3, Warning, TEXT("SetVariable: invalid payload."));
        return;
    }
    if (!Ctx.VariableStore)
    {
        UE_LOG(LogIMOPExecSetVarV3, Error, TEXT("SetVariable: missing VariableStore."));
        return;
    }

    Ctx.VariableStore->SetValue(P->Key, P->Value);
}

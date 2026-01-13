#include "Actions/Exec_ModifyVariableV3.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "Stores/SpellVariableStoreV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecModVarV3, Log, All);

const UScriptStruct* UExec_ModifyVariableV3::GetPayloadStruct() const
{
    return FPayload_ModifyVariableV3::StaticStruct();
}

void UExec_ModifyVariableV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
    const FPayload_ModifyVariableV3* P = static_cast<const FPayload_ModifyVariableV3*>(PayloadData);
    if (!P || P->Key == NAME_None)
    {
        UE_LOG(LogIMOPExecModVarV3, Warning, TEXT("ModifyVariable: invalid payload."));
        return;
    }
    if (!Ctx.VariableStore)
    {
        UE_LOG(LogIMOPExecModVarV3, Error, TEXT("ModifyVariable: missing VariableStore."));
        return;
    }

    const bool bOk = Ctx.VariableStore->ModifyFloat(P->Key, P->Op, P->Operand);
    if (!bOk)
    {
        UE_LOG(LogIMOPExecModVarV3, Warning, TEXT("ModifyVariable: failed (type mismatch or invalid op). Key=%s"), *P->Key.ToString());
    }
}

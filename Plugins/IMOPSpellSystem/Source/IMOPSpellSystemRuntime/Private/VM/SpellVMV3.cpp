#include "VM/SpellVMV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "Actions/SpellActionExecutorV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPVMV3, Log, All);

void FSpellVMV3::ExecuteActions(USpellRuntimeV3& Runtime, const TArray<FSpellActionV3>& Actions)
{
    USpellActionRegistryV3* Reg = Runtime.BuildExecContext().ActionRegistry;
    if (!Reg)
    {
        UE_LOG(LogIMOPVMV3, Error, TEXT("ExecuteActions: missing ActionRegistry."));
        return;
    }

    const FSpellExecContextV3 Ctx = Runtime.BuildExecContext();

    for (const FSpellActionV3& A : Actions)
    {
        if (!A.ActionTag.IsValid())
        {
            UE_LOG(LogIMOPVMV3, Warning, TEXT("Action has invalid tag, skipped."));
            continue;
        }

        const FRegisteredActionBindingV3* Binding = Reg->FindBinding(A.ActionTag);
        if (!Binding)
        {
            UE_LOG(LogIMOPVMV3, Error, TEXT("No binding for ActionTag: %s"), *A.ActionTag.ToString());
            continue;
        }

        // Validate payload type
        const UScriptStruct* Expected = Binding->PayloadStruct;
        const UScriptStruct* Actual = A.Payload.GetScriptStruct();
        if (Expected && Actual && Expected != Actual)
        {
            UE_LOG(LogIMOPVMV3, Error, TEXT("Payload mismatch for %s: expected %s got %s"),
                *A.ActionTag.ToString(),
                *Expected->GetName(),
                *Actual->GetName());
            continue;
        }

        USpellActionExecutorV3* Exec = Runtime.GetOrCreateExecutor(Binding->ExecutorClass);
        if (!Exec)
        {
            UE_LOG(LogIMOPVMV3, Error, TEXT("Failed to create executor for %s"), *A.ActionTag.ToString());
            continue;
        }

        const void* PayloadData = A.Payload.GetMemory();
        Exec->Execute(Ctx, PayloadData);
    }
}

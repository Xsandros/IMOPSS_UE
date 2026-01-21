#include "VM/SpellVMV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "Actions/SpellActionExecutorV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPVMV3, Log, All);

void FSpellVMV3::ExecuteActions(USpellRuntimeV3& Runtime, const TArray<FSpellActionV3>& Actions)
{
	const FSpellExecContextV3 Ctx = Runtime.BuildExecContext();
	USpellActionRegistryV3* Reg = Ctx.ActionRegistry;

	if (!Reg)
	{
		UE_LOG(LogIMOPVMV3, Error, TEXT("ExecuteActions: missing ActionRegistry."));
		return;
	}

	for (int32 Index = 0; Index < Actions.Num(); ++Index)
	{
		const FSpellActionV3& A = Actions[Index];

		if (!A.ActionTag.IsValid())
		{
			UE_LOG(LogIMOPVMV3, Warning, TEXT("Action SKIP: idx=%d invalid ActionTag"), Index);
			continue;
		}

		// Policy gate
		if (EnumHasAnyFlags(A.Policy, ESpellActionPolicyV3::RequireAuthority) && !Ctx.bAuthority)
		{
			UE_LOG(LogIMOPVMV3, Verbose, TEXT("Action SKIP (not authority): idx=%d tag=%s"), Index, *A.ActionTag.ToString());
			continue;
		}

		const FRegisteredActionBindingV3* Binding = Reg->FindBinding(A.ActionTag);
		if (!Binding)
		{
			UE_LOG(LogIMOPVMV3, Error, TEXT("Action SKIP: idx=%d no binding for ActionTag=%s"), Index, *A.ActionTag.ToString());
			continue;
		}

		const UScriptStruct* Expected = Binding->PayloadStruct;
		const UScriptStruct* Actual = A.Payload.GetScriptStruct();

		const FString ExpectedName = Expected ? Expected->GetName() : TEXT("None");
		const FString ActualName = Actual ? Actual->GetName() : TEXT("None");


		UE_LOG(LogIMOPVMV3, Log,
			TEXT("Action CALL: idx=%d tag=%s payload=%s policy=%d (expected=%s exec=%s)"),
			Index,
			*A.ActionTag.ToString(),
			*ActualName,
			(int32)A.Policy,
			*ExpectedName,
			Binding->ExecutorClass ? *Binding->ExecutorClass->GetName() : TEXT("None"));

		// Validate payload type (strict when both are present)
		if (Expected && Actual && Expected != Actual)
		{
			UE_LOG(LogIMOPVMV3, Error,
				TEXT("Action SKIP: idx=%d payload mismatch tag=%s expected=%s got=%s"),
				Index,
				*A.ActionTag.ToString(),
				*ExpectedName,
				*ActualName);
			continue;
		}

		USpellActionExecutorV3* Exec = Runtime.GetOrCreateExecutor(Binding->ExecutorClass);
		if (!Exec)
		{
			UE_LOG(LogIMOPVMV3, Error, TEXT("Action SKIP: idx=%d failed to create executor for tag=%s"), Index, *A.ActionTag.ToString());
			continue;
		}

		const void* PayloadData = A.Payload.GetMemory();
		Exec->Execute(Ctx, PayloadData);

		UE_LOG(LogIMOPVMV3, Log, TEXT("Action DONE: idx=%d tag=%s"), Index, *A.ActionTag.ToString());
	}
}


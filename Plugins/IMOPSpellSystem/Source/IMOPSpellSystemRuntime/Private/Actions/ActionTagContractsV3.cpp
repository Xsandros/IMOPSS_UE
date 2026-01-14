#include "Actions/ActionTagContractsV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "Core/SpellGameplayTagsV3.h"
#include "Actions/SpellPayloadsCoreV3.h"

// Private-only exec headers (implementation detail)
#include "Actions/Exec_EmitEventV3.h"
#include "Actions/Exec_SetVariableV3.h"
#include "Actions/Exec_ModifyVariableV3.h"
#include "Actions/Exec_SetTargetSetV3.h"
#include "Actions/Exec_ClearTargetSetV3.h"
// Targeting
#include "Targeting/SpellPayloadsTargetingV3.h"
#include "Targeting/Exec_TargetAcquireV3.h"
#include "Targeting/Exec_TargetFilterV3.h"
#include "Targeting/Exec_TargetSelectV3.h"

// Effects
#include "Effects/SpellPayloadsEffectsV3.h"
#include "Effects/Exec_ModifyAttributeV3.h"
#include "Effects/Exec_ReadAttributeV3.h"
#include "Effects/Exec_ApplyForceV3.h"

// Status
#include "Status/SpellPayloadsStatusV3.h"
#include "Status/Exec_ApplyStatusV3.h"
#include "Status/Exec_RemoveStatusV3.h"



DEFINE_LOG_CATEGORY_STATIC(LogIMOPContractsV3, Log, All);

static TArray<FActionBindingV3> BuildDefaultBindings()
{
	const auto& Tags = FIMOPSpellGameplayTagsV3::Get();
	TArray<FActionBindingV3> Binds;
	Binds.Reserve(14);

	Binds.Add({ Tags.Action_EmitEvent,      UExec_EmitEventV3::StaticClass(),      FPayload_EmitEventV3::StaticStruct() });
	Binds.Add({ Tags.Action_SetVariable,    UExec_SetVariableV3::StaticClass(),    FPayload_SetVariableV3::StaticStruct() });
	Binds.Add({ Tags.Action_ModifyVariable, UExec_ModifyVariableV3::StaticClass(), FPayload_ModifyVariableV3::StaticStruct() });
	Binds.Add({ Tags.Action_SetTargetSet,   UExec_SetTargetSetV3::StaticClass(),   FPayload_SetTargetSetV3::StaticStruct() });
	Binds.Add({ Tags.Action_ClearTargetSet, UExec_ClearTargetSetV3::StaticClass(), FPayload_ClearTargetSetV3::StaticStruct() });
	Binds.Add({ Tags.Action_Targeting_Acquire, UExec_TargetAcquireV3::StaticClass(), FPayload_TargetAcquireV3::StaticStruct() });
	Binds.Add({ Tags.Action_Targeting_Filter,  UExec_TargetFilterV3::StaticClass(),  FPayload_TargetFilterV3::StaticStruct() });
	Binds.Add({ Tags.Action_Targeting_Select,  UExec_TargetSelectV3::StaticClass(),  FPayload_TargetSelectV3::StaticStruct() });
	// Effects
	Binds.Add({ Tags.Action_Effect_ModifyAttribute, UExec_ModifyAttributeV3::StaticClass(), FPayload_EffectModifyAttributeV3::StaticStruct() });
	Binds.Add({ Tags.Action_Effect_ReadAttribute,   UExec_ReadAttributeV3::StaticClass(),   FPayload_ReadAttributeV3::StaticStruct() });
	Binds.Add({ Tags.Action_Effect_ApplyForce,      UExec_ApplyForceV3::StaticClass(),      FPayload_EffectApplyForceV3::StaticStruct() });

	// Status
	Binds.Add({ Tags.Action_Status_Apply,           UExec_ApplyStatusV3::StaticClass(),     FPayload_ApplyStatusV3::StaticStruct() });
	Binds.Add({ Tags.Action_Status_Remove,          UExec_RemoveStatusV3::StaticClass(),    FPayload_RemoveStatusV3::StaticStruct() });

	
	return Binds;
}

void FActionTagContractsV3::RegisterDefaults(USpellActionRegistryV3& Registry)
{
	const TArray<FActionBindingV3> Defaults = BuildDefaultBindings();

	int32 Registered = 0;

	for (const FActionBindingV3& B : Defaults)
	{
		FText Error;
		if (!ValidateBinding(B.ActionTag, B.ExecutorClass, B.PayloadStruct, Error))
		{
			UE_LOG(LogIMOPContractsV3, Error, TEXT("Invalid default binding: %s"), *Error.ToString());
			continue;
		}

		if (Registry.RegisterBinding(B.ActionTag, B.ExecutorClass, B.PayloadStruct, /*bAllowOverride*/ true, &Error))
		{
			Registered++;
		}
		else
		{
			UE_LOG(LogIMOPContractsV3, Warning, TEXT("Failed to register binding for %s: %s"), *B.ActionTag.ToString(), *Error.ToString());
		}
	}

	UE_LOG(LogIMOPContractsV3, Log, TEXT("Registered %d default Action bindings."), Registered);
}

bool FActionTagContractsV3::ValidateBinding(
	const FGameplayTag& ActionTag,
	TSubclassOf<USpellActionExecutorV3> ExecutorClass,
	const UScriptStruct* PayloadStruct,
	FText& OutError)
{
	if (!ActionTag.IsValid())
	{
		OutError = FText::FromString(TEXT("ActionTag is invalid."));
		return false;
	}
	if (!ExecutorClass)
	{
		OutError = FText::FromString(TEXT("ExecutorClass is null."));
		return false;
	}
	if (PayloadStruct == nullptr)
	{
		OutError = FText::FromString(TEXT("PayloadStruct is null."));
		return false;
	}

	// Optional sanity: ensure payload is a UStruct (it is), and not abstract, etc.
	return true;
}

const UScriptStruct* FActionTagContractsV3::ExpectedPayloadStruct(const FGameplayTag& ActionTag)
{
	const TArray<FActionBindingV3> Defaults = BuildDefaultBindings();
	for (const FActionBindingV3& B : Defaults)
	{
		if (B.ActionTag == ActionTag)
		{
			return B.PayloadStruct;
		}
	}
	return nullptr;
}

void FActionTagContractsV3::DumpDefaultsToLog()
{
	const TArray<FActionBindingV3> Defaults = BuildDefaultBindings();

	UE_LOG(LogIMOPContractsV3, Log, TEXT("=== Default Action Contracts (Phase 0) ==="));
	for (const FActionBindingV3& B : Defaults)
	{
		const FString PayloadName = B.PayloadStruct ? B.PayloadStruct->GetName() : TEXT("<null>");
		UE_LOG(LogIMOPContractsV3, Log, TEXT("Action=%s  Exec=%s  Payload=%s"),
			*B.ActionTag.ToString(),
			B.ExecutorClass ? *B.ExecutorClass->GetName() : TEXT("<null>"),
			*PayloadName
		);
	}
}

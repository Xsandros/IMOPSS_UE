#include "Delivery/Exec_StartDeliveryV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/SpellPayloadsDeliveryV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Logging/LogMacros.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecStartDeliveryV3, Log, All);

void UExec_StartDeliveryV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	if (!PayloadData)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("StartDelivery Execute: PayloadData null"));
		return;
	}

	const FPayload_DeliveryStartV3* P = static_cast<const FPayload_DeliveryStartV3*>(PayloadData);

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("StartDelivery Execute: World null"));
		return;
	}

	UDeliverySubsystemV3* Sub = World->GetSubsystem<UDeliverySubsystemV3>();
	if (!Sub)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("StartDelivery Execute: DeliverySubsystem missing"));
		return;
	}

	// ---- Fallback: auto-generate a DeliveryId if authoring forgot it ----
	FDeliverySpecV3 SpecToStart = P->Spec;

	if (SpecToStart.DeliveryId == NAME_None)
	{
		FName BaseId = NAME_None;

		if (Ctx.Runtime && Ctx.Runtime->GetSpec())
		{
			const USpellSpecV3* SpellSpec = Ctx.Runtime->GetSpec();
			BaseId = (SpellSpec->SpellId != NAME_None)
				? SpellSpec->SpellId
				: FName(*SpellSpec->GetName());
		}
		else
		{
			BaseId = FName(TEXT("Spell"));
		}

		FString Suffix(TEXT("Auto"));
		if (Ctx.Runtime)
		{
			const FString GuidStr =
				Ctx.Runtime->GetRuntimeGuid().ToString(EGuidFormats::Digits);
			Suffix = FString::Printf(TEXT("Auto_%s"), *GuidStr.Left(8));
		}

		const FString AutoIdStr =
			FString::Printf(TEXT("%s_%s"), *BaseId.ToString(), *Suffix);

		SpecToStart.DeliveryId = FName(*AutoIdStr);

		UE_LOG(LogIMOPExecStartDeliveryV3, Warning,
			TEXT("StartDelivery: Spec.DeliveryId was None -> using fallback '%s'"),
			*SpecToStart.DeliveryId.ToString());
	}
	// ---- end fallback ----



	// Composite-first sanity
	if (P->Spec.Primitives.Num() <= 0)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("StartDelivery: Spec.Primitives is empty (Composite-first). DeliveryId=%s"),
			*P->Spec.DeliveryId.ToString());
		return;
	}

	FDeliveryHandleV3 Handle;
	const bool bOk = Sub->StartDelivery(Ctx, P->Spec, Handle);

	UE_LOG(LogIMOPExecStartDeliveryV3, Log, TEXT("StartDelivery: id=%s ok=%d inst=%d"),
		*P->Spec.DeliveryId.ToString(),
		bOk ? 1 : 0,
		Handle.InstanceIndex);

	// NOTE: storing the handle in VariableStore should be a dedicated Exec (keeps this deterministic + simple)
}

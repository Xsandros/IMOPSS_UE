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

	if (P->Spec.DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("StartDelivery: Spec.DeliveryId is None"));
		return;
	}

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

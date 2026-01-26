#include "Delivery/Exec_StopDeliveryV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/SpellPayloadsDeliveryV3.h"

#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecStopDeliveryV3, Log, All);

void UExec_StopDeliveryV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	if (!PayloadData)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("StopDelivery Execute: PayloadData null"));
		return;
	}

	const FPayload_DeliveryStopV3* P = static_cast<const FPayload_DeliveryStopV3*>(PayloadData);

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("StopDelivery Execute: World null"));
		return;
	}

	UDeliverySubsystemV3* Sub = World->GetSubsystem<UDeliverySubsystemV3>();
	if (!Sub)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("StopDelivery Execute: DeliverySubsystem missing"));
		return;
	}

	bool bOk = false;

	if (P->bUseHandle && P->Handle.RuntimeGuid.IsValid())
	{
		bOk = Sub->StopDelivery(Ctx, P->Handle, P->Reason);

		UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by Handle: id=%s inst=%d ok=%d"),
			*P->Handle.DeliveryId.ToString(),
			P->Handle.InstanceIndex,
			bOk ? 1 : 0);
		return;
	}

	if (P->DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Warning, TEXT("StopDelivery: No handle and DeliveryId None -> nothing to stop"));
		return;
	}

	if (P->bUsePrimitiveId && P->PrimitiveId != NAME_None)
	{
		bOk = Sub->StopByPrimitiveId(Ctx, P->DeliveryId, P->PrimitiveId, P->Reason);

		UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by PrimitiveId: delivery=%s primitive=%s ok=%d"),
			*P->DeliveryId.ToString(),
			*P->PrimitiveId.ToString(),
			bOk ? 1 : 0);
		return;
	}

	bOk = Sub->StopById(Ctx, P->DeliveryId, P->Reason);

	UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by DeliveryId: delivery=%s ok=%d"),
		*P->DeliveryId.ToString(),
		bOk ? 1 : 0);
}

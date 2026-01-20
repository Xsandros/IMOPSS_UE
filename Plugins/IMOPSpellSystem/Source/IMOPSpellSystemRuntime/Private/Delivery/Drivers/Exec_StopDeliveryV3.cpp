#include "Delivery/Exec_StopDeliveryV3.h"
#include "Delivery/DeliverySubsystemV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecDeliveryV3, Log, All);

void UExec_StopDeliveryV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_DeliveryStopV3* P = static_cast<const FPayload_DeliveryStopV3*>(PayloadData);
	if (!P)
	{
		UE_LOG(LogIMOPExecDeliveryV3, Error, TEXT("Exec_StopDelivery: PayloadData null"));
		return;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPExecDeliveryV3, Error, TEXT("Exec_StopDelivery: World null"));
		return;
	}

	UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>();
	if (!Subsys)
	{
		UE_LOG(LogIMOPExecDeliveryV3, Error, TEXT("Exec_StopDelivery: DeliverySubsystem missing"));
		return;
	}

	bool bStopped = false;
	if (P->bUseHandle && P->Handle.IsValid())
	{
		bStopped = Subsys->StopDelivery(Ctx, P->Handle, P->Reason);
	}
	else if (P->DeliveryId != NAME_None)
	{
		bStopped = Subsys->StopById(Ctx, P->DeliveryId, P->Reason);
	}

	UE_LOG(LogIMOPExecDeliveryV3, Log, TEXT("Exec_StopDelivery: stopped=%d"), bStopped ? 1 : 0);
}

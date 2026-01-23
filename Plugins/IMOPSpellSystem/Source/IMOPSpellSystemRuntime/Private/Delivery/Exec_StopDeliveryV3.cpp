#include "Delivery/Exec_StopDeliveryV3.h"
#include "Delivery/DeliverySubsystemV3.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecStopDeliveryV3, Log, All);

static const TCHAR* StopReasonToString(EDeliveryStopReasonV3 R)
{
	switch (R)
	{
	case EDeliveryStopReasonV3::Manual:          return TEXT("Manual");
	case EDeliveryStopReasonV3::DurationElapsed: return TEXT("DurationElapsed");
	case EDeliveryStopReasonV3::OnFirstHit:      return TEXT("OnFirstHit");
	case EDeliveryStopReasonV3::OnEvent:         return TEXT("OnEvent");
	case EDeliveryStopReasonV3::OwnerDestroyed:  return TEXT("OwnerDestroyed");
	default:                                     return TEXT("Unknown");
	}
}

void UExec_StopDeliveryV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_DeliveryStopV3* P = static_cast<const FPayload_DeliveryStopV3*>(PayloadData);
	if (!P)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("Exec_StopDelivery: PayloadData null"));
		return;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("Exec_StopDelivery: World null"));
		return;
	}

	UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>();
	if (!Subsys)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Error, TEXT("Exec_StopDelivery: DeliverySubsystem missing"));
		return;
	}

	const bool bHasValidHandle = (P->bUseHandle && P->Handle.IsValid());
	const bool bHasId = (P->DeliveryId != NAME_None);

	if (!bHasValidHandle && !bHasId)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Warning,
			TEXT("Exec_StopDelivery: No valid handle or DeliveryId specified. bUseHandle=%d HandleValid=%d DeliveryId=None Reason=%s"),
			P->bUseHandle ? 1 : 0,
			P->Handle.IsValid() ? 1 : 0,
			StopReasonToString(P->Reason));
		return;
	}

	bool bStopped = false;

	if (P->bUseHandle && P->Handle.IsValid())
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Log,
			TEXT("Exec_StopDelivery: StopDelivery by Handle Id=%s Inst=%d Reason=%s"),
			*P->Handle.DeliveryId.ToString(),
			P->Handle.InstanceIndex,
			StopReasonToString(P->Reason));

		bStopped = Subsys->StopDelivery(Ctx, P->Handle, P->Reason);
	}
	else if (P->bUsePrimitiveId && P->PrimitiveId != NAME_None)
	{
		// DeliveryId can be None => search all deliveries in this runtime
		bStopped = Subsys->StopByPrimitiveId(Ctx, P->DeliveryId, P->PrimitiveId, P->Reason);
	}
	else if (P->DeliveryId != NAME_None)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Log,
			TEXT("Exec_StopDelivery: StopById Id=%s Reason=%s"),
			*P->DeliveryId.ToString(),
			StopReasonToString(P->Reason));

		bStopped = Subsys->StopById(Ctx, P->DeliveryId, P->Reason);
	}

	UE_LOG(LogIMOPExecStopDeliveryV3, Log,
		TEXT("Exec_StopDelivery: stopped=%d reason=%s"),
		bStopped ? 1 : 0,
		StopReasonToString(P->Reason));
}

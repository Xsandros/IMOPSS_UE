#include "Delivery/Exec_StartDeliveryV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/SpellPayloadsDeliveryV3.h"

#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecStartDeliveryV3, Log, All);

void UExec_StartDeliveryV3::Execute(const FSpellExecContextV3& Ctx, const void* PayloadData)
{
	const FPayload_DeliveryStartV3* P = static_cast<const FPayload_DeliveryStartV3*>(PayloadData);
	if (!P)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("Exec_StartDelivery: PayloadData null"));
		return;
	}

	UWorld* World = Ctx.GetWorld();
	if (!World)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("Exec_StartDelivery: World null"));
		return;
	}

	UDeliverySubsystemV3* Subsys = World->GetSubsystem<UDeliverySubsystemV3>();
	if (!Subsys)
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Error, TEXT("Exec_StartDelivery: DeliverySubsystem missing"));
		return;
	}

	FDeliveryHandleV3 Handle;
	if (!Subsys->StartDelivery(Ctx, P->Spec, Handle))
	{
		UE_LOG(LogIMOPExecStartDeliveryV3, Warning, TEXT("Exec_StartDelivery: StartDelivery failed (Id=%s)"),
			*P->Spec.DeliveryId.ToString());
		return;
	}

	UE_LOG(LogIMOPExecStartDeliveryV3, Log, TEXT("Exec_StartDelivery: started Id=%s Inst=%d"),
		*Handle.DeliveryId.ToString(), Handle.InstanceIndex);
}

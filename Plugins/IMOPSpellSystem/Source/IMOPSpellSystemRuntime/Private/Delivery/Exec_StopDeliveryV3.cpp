#include "Delivery/Exec_StopDeliveryV3.h"

#include "Delivery/DeliverySubsystemV3.h"
#include "Delivery/SpellPayloadsDeliveryV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Logging/LogMacros.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPExecStopDeliveryV3, Log, All);

static FGuid ResolveRuntimeGuidFromCtx_StopExec(const FSpellExecContextV3& Ctx)
{
	if (const USpellRuntimeV3* R = Cast<USpellRuntimeV3>(Ctx.Runtime.Get()))
	{
		return R->GetRuntimeGuid();
	}
	return FGuid();
}

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

	const FGuid RuntimeGuid = ResolveRuntimeGuidFromCtx_StopExec(Ctx);

	bool bOk = false;

	// 0) Stop by primitive handle (most precise: group instance + primitive)
	if (P->bUsePrimitiveHandle)
	{
		FDeliveryPrimitiveHandleV3 PH = P->PrimitiveHandle;

		// Robustness: if payload doesn't carry guid, use ctx runtime guid
		if (!PH.Group.RuntimeGuid.IsValid() && RuntimeGuid.IsValid())
		{
			PH.Group.RuntimeGuid = RuntimeGuid;
		}

		if (!PH.Group.RuntimeGuid.IsValid())
		{
			UE_LOG(LogIMOPExecStopDeliveryV3, Warning,
				TEXT("StopDelivery by PrimitiveHandle requested, but RuntimeGuid is invalid (payload and ctx)."));
			return;
		}

		if (PH.Group.DeliveryId == NAME_None)
		{
			UE_LOG(LogIMOPExecStopDeliveryV3, Warning,
				TEXT("StopDelivery by PrimitiveHandle requested, but DeliveryId is None."));
			return;
		}

		if (PH.PrimitiveId == NAME_None)
		{
			UE_LOG(LogIMOPExecStopDeliveryV3, Warning,
				TEXT("StopDelivery by PrimitiveHandle requested, but PrimitiveId is None."));
			return;
		}

		const bool bStopped = Sub->StopPrimitive(Ctx, PH, P->Reason);
		UE_LOG(LogIMOPExecStopDeliveryV3, Log,
			TEXT("StopDelivery by PrimitiveHandle: id=%s inst=%d prim=%s ok=%d"),
			*PH.Group.DeliveryId.ToString(), PH.Group.InstanceIndex, *PH.PrimitiveId.ToString(), bStopped ? 1 : 0);
		return;
	}

	
	// 1) Stop by explicit handle (most precise)
	if (P->bUseHandle)
	{
		FDeliveryHandleV3 H = P->Handle;

		// Robustness: if the payload doesn't carry a guid, use the one from this runtime context.
		if (!H.RuntimeGuid.IsValid() && RuntimeGuid.IsValid())
		{
			H.RuntimeGuid = RuntimeGuid;
		}

		if (!H.RuntimeGuid.IsValid())
		{
			UE_LOG(LogIMOPExecStopDeliveryV3, Warning, TEXT("StopDelivery by Handle requested, but RuntimeGuid is invalid (payload and ctx)."));
			return;
		}

		bOk = Sub->StopDelivery(Ctx, H, P->Reason);

		UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by Handle: id=%s inst=%d ok=%d"),
			*H.DeliveryId.ToString(),
			H.InstanceIndex,
			bOk ? 1 : 0);
		return;
	}

	// 2) Stop by ids (less precise, but authoring-friendly)
	if (P->DeliveryId == NAME_None)
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Warning, TEXT("StopDelivery: No handle and DeliveryId is None -> nothing to stop"));
		return;
	}

	// IMPORTANT: We prefer to stop within the current runtime only.
	// We do this by iterating active handles and calling StopDelivery with exact handles.
	// This prevents accidental cross-runtime stops in MP when multiple spell runtimes exist.
	TArray<FDeliveryHandleV3> Handles;
	Sub->GetActiveHandles(Handles);

	if (!RuntimeGuid.IsValid())
	{
		UE_LOG(LogIMOPExecStopDeliveryV3, Warning, TEXT("StopDelivery: Ctx has no valid RuntimeGuid; will match by DeliveryId only (potentially broader)."));
	}

	if (P->bUsePrimitiveId && P->PrimitiveId != NAME_None)
	{
		// Best-effort: subsystem has StopByPrimitiveId, but to keep runtime scoping tight we first filter by handles.
		bool bAny = false;

		for (const FDeliveryHandleV3& H : Handles)
		{
			if (H.DeliveryId != P->DeliveryId)
			{
				continue;
			}
			if (RuntimeGuid.IsValid() && H.RuntimeGuid != RuntimeGuid)
			{
				continue;
			}

			// Scoped call (still uses subsystem helper internally)
			bAny |= Sub->StopByPrimitiveId(Ctx, P->DeliveryId, P->PrimitiveId, P->Reason);
			break; // primitive id is unique per group spec; one stop is enough
		}

		UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by PrimitiveId: delivery=%s primitive=%s ok=%d"),
			*P->DeliveryId.ToString(),
			*P->PrimitiveId.ToString(),
			bAny ? 1 : 0);
		return;
	}

	// Stop whole delivery groups with matching DeliveryId within this runtime
	{
		bool bAny = false;
		for (const FDeliveryHandleV3& H : Handles)
		{
			if (H.DeliveryId != P->DeliveryId)
			{
				continue;
			}
			if (RuntimeGuid.IsValid() && H.RuntimeGuid != RuntimeGuid)
			{
				continue;
			}

			bAny |= Sub->StopDelivery(Ctx, H, P->Reason);
		}

		UE_LOG(LogIMOPExecStopDeliveryV3, Log, TEXT("StopDelivery by DeliveryId: delivery=%s ok=%d"),
			*P->DeliveryId.ToString(),
			bAny ? 1 : 0);
	}
}

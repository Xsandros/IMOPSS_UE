#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "DeliveryDriverBaseV3.generated.h"

struct FSpellExecContextV3;
class UDeliveryGroupRuntimeV3;

/**
 * Base class for all Delivery drivers (Composite-first).
 *
 * Lifecycle:
 * - Start(Ctx, Group, PrimitiveCtx)
 * - Tick(Ctx, Group, DeltaSeconds)  (optional)
 * - Stop(Ctx, Group, Reason)
 *
 * Notes:
 * - Drivers are owned by DeliverySubsystemV3 / GroupRuntime and are server-authoritative.
 * - Drivers keep minimal state + can read/write Group->Blackboard and Group->PrimitiveCtxById.
 */
UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriverBaseV3 : public UObject
{
	GENERATED_BODY()

public:
	// Set by subsystem before Start()
	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FName PrimitiveId = NAME_None;

	UPROPERTY()
	bool bActive = false;

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) PURE_VIRTUAL(UDeliveryDriverBaseV3::Start, );
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) {}
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) PURE_VIRTUAL(UDeliveryDriverBaseV3::Stop, );

	bool IsActive() const { return bActive; }
};

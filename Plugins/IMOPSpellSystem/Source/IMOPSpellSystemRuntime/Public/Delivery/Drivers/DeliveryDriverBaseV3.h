#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "DeliveryDriverBaseV3.generated.h"

class UDeliveryGroupRuntimeV3;
struct FSpellExecContextV3;

/**
 * Base class for Delivery drivers.
 *
 * Composite-first:
 * - One GroupRuntime per StartDelivery.
 * - One Driver per PrimitiveId inside that group.
 *
 * Drivers are pure logic (no actors required), deterministic on server.
 * Rendering/VFX will be handled later (Phase 12) via Presentation hooks in specs.
 */
UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriverBaseV3 : public UObject
{
	GENERATED_BODY()

public:
	// Identity (set by subsystem)
	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FName PrimitiveId = NAME_None;

	UPROPERTY()
	bool bActive = false;

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) {}
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) {}
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) {}

	bool IsActive() const { return bActive; }
};

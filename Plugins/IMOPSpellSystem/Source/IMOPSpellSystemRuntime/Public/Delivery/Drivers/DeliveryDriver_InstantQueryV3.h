#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_InstantQueryV3.generated.h"

/**
 * InstantQuery driver (Composite-first):
 * - Evaluates once on Start (no Tick needed).
 * - Uses Ray shape semantics (line trace) by default; if shape is Sphere/Box/Capsule we do sweep.
 * - Writes hits to TargetStore if OutTargetSetName is set (or group default).
 * - Debug draw for path + hits.
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_InstantQueryV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) override {}
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) override;

private:
	UPROPERTY()
	FDeliveryContextV3 LocalCtx;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From);

	bool EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
};

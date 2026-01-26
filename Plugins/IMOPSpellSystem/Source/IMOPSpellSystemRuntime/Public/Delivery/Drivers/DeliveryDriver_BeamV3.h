#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_BeamV3.generated.h"

/**
 * Beam driver (Composite-first):
 * - Periodically traces from PrimitiveCtx.FinalPoseWS forward (or towards lock target).
 * - Uses LineTraceMultiByProfile when Radius == 0, otherwise Sphere SweepMultiByProfile.
 * - Debug draws beam + hits.
 * - Optional: writes current hit list into TargetStore (OutTargetSetName / group default).
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_BeamV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) override;

private:
	UPROPERTY()
	FDeliveryContextV3 LocalCtx;

	UPROPERTY()
	float NextEvalTimeSeconds = 0.f;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From);

	bool EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	// Lock-on aim (optional)
	bool TryResolveLockTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryBeamConfigV3& BeamCfg, FVector& OutTargetLoc) const;
};

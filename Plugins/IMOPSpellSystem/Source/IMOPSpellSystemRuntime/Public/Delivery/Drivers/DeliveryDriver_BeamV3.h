#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"

#include "DeliveryDriver_BeamV3.generated.h"

class UDeliveryGroupRuntimeV3;

/**
 * Beam driver (Composite-first)
 * - Ticks at Spec.Beam.TickInterval (0 => every tick)
 * - Origin/Dir comes from PrimitiveCtx.FinalPoseWS (already includes AnchorRef)
 * - Optional LockOn: aim towards best target from LockTargetSet
 * - Performs LineTraceMulti (Radius==0) or SphereSweepMulti (Radius>0)
 * - Tracks InsideSet for Enter/Exit/Stay
 * - Writes hit actors into TargetStore (Primitive OutTargetSetName or Group default)
 * - Emits Primitive events: Started/Stopped/Tick/Enter/Exit/Stay/Hit
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
	float TimeSinceLastEval = 0.f;

	// Persistent set for Enter/Exit/Stay semantics
	TSet<TWeakObjectPtr<AActor>> InsideSet;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	static void SortActorsDeterministic(TArray<AActor*>& Actors);

	static AActor* PickLockTargetDeterministic(const FSpellExecContextV3& Ctx, FName TargetSetName);

	void EvaluateBeam(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group);
};

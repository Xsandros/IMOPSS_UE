#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_MoverV3.generated.h"

/**
 * Mover driver (Composite-first):
 * - Simulates a projectile-like mover deterministically on the server (and in replays).
 * - Uses PrimitiveCtx.FinalPoseWS as spawn pose, then integrates forward.
 * - Optional: homing uses TargetStore (HomingTargetSet) to pick first target.
 * - Optional: sweep hit test along segment each tick/interval.
 * - Debug draw for path + hits.
 *
 * NOTE: For now, we keep mover state inside the driver (not in blackboard).
 *       In later phases, you can migrate state to group blackboard for replication snapshots.
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_MoverV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) override;

private:
	UPROPERTY()
	FDeliveryContextV3 LocalCtx;

	// Current mover state
	UPROPERTY()
	FVector PosWS = FVector::ZeroVector;

	UPROPERTY()
	FVector VelWS = FVector::ZeroVector;

	UPROPERTY()
	float DistanceTraveled = 0.f;

	UPROPERTY()
	float NextSimTimeSeconds = 0.f;

	UPROPERTY()
	int32 PierceCount = 0;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	bool TryResolveHomingTargetLocation(const FSpellExecContextV3& Ctx, const struct FDeliveryMoverConfigV3& MoverCfg, FVector& OutTargetLoc) const;

	bool StepSim(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float StepDt);
	bool DoSweepHit(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FVector& From, const FVector& To, TArray<FHitResult>& OutHits) const;

	void DebugDrawStep(UWorld* World, const struct FDeliveryDebugDrawConfigV3& DebugCfg, const FVector& From, const FVector& To) const;
};

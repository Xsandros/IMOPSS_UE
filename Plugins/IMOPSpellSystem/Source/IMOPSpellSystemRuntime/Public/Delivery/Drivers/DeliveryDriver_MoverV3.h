#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"

#include "DeliveryDriver_MoverV3.generated.h"

class UDeliveryGroupRuntimeV3;

/**
 * Mover driver (Composite-first)
 * - Owns per-primitive motion state (position/velocity/traveled).
 * - Updates Group->PrimitiveCtxById[PrimitiveId].FinalPoseWS so other systems can read live pose.
 * - Performs collision each sim step (line trace for Ray/LineTrace, sweep otherwise; overlap if requested).
 * - Emits Primitive events: Started / Tick / Hit / Stopped.
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

	// Motion state (world space)
	UPROPERTY()
	FVector PositionWS = FVector::ZeroVector;

	UPROPERTY()
	FVector VelocityWS = FVector::ZeroVector;

	UPROPERTY()
	float TraveledDistance = 0.f;

	UPROPERTY()
	int32 PierceHits = 0;

	// For fixed tick interval stepping
	UPROPERTY()
	float NextSimTimeSeconds = 0.f;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From);
	static FCollisionShape MakeCollisionShape(const FDeliveryShapeV3& Shape);

	bool GetHomingTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryMoverConfigV3& M, FVector& OutLoc) const;

	bool SweepMoveAndCollectHits(
		const FSpellExecContextV3& Ctx,
		UWorld* World,
		const FDeliveryPrimitiveSpecV3& Spec,
		const FVector& From,
		const FVector& To,
		TArray<FHitResult>& OutHits
	) const;

	void DebugDraw(
		const FSpellExecContextV3& Ctx,
		const UDeliveryGroupRuntimeV3* Group,
		const FDeliveryPrimitiveSpecV3& Spec,
		const FVector& From,
		const FVector& To,
		const TArray<FHitResult>& Hits
	) const;

	void WriteHitsToTargetStore(
		const FSpellExecContextV3& Ctx,
		const UDeliveryGroupRuntimeV3* Group,
		const FDeliveryContextV3& PrimitiveCtx,
		const TArray<FHitResult>& Hits
	) const;

	bool StepSim(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float StepSeconds);
};

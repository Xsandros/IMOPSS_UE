#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_MoverV3.generated.h"

/**
 * Mover driver (Composite-first):
 * - Simulates a moving primitive (projectile-ish) in world space.
 * - Motion modes: Straight / Ballistic / Homing (config in Spec.Mover).
 * - Performs sweep along movement step using Spec.Shape + Spec.Query.
 * - Optional pierce; optional stop-on-hit.
 * - Writes hit targets to TargetStore (OutTargetSetName / group default).
 * - Debug draw path + hits + shape.
 *
 * Determinism: server authoritative, deterministic ordering + no RNG.
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

	// Sim state
	UPROPERTY()
	FVector PositionWS = FVector::ZeroVector;

	UPROPERTY()
	FVector VelocityWS = FVector::ZeroVector;

	UPROPERTY()
	float TraveledDistance = 0.f;

	UPROPERTY()
	int32 PierceHits = 0;

	UPROPERTY()
	float NextSimTimeSeconds = 0.f;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From);

	static FCollisionShape MakeCollisionShape(const FDeliveryShapeV3& Shape);
	static FQuat RotationForSweep(const FDeliveryShapeV3& Shape, const FTransform& Pose);

	bool GetHomingTargetLocation(const FSpellExecContextV3& Ctx, const FDeliveryMoverConfigV3& M, FVector& OutLoc) const;

	bool StepSim(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float StepSeconds);
	bool SweepMoveAndCollectHits(
		const FSpellExecContextV3& Ctx,
		UWorld* World,
		const FDeliveryPrimitiveSpecV3& Spec,
		const FVector& From,
		const FVector& To,
		TArray<FHitResult>& OutHits
	) const;

	void DebugDraw(const FSpellExecContextV3& Ctx, const UDeliveryGroupRuntimeV3* Group, const FDeliveryPrimitiveSpecV3& Spec, const FVector& From, const FVector& To, const TArray<FHitResult>& Hits) const;
	void WriteHitsToTargetStore(const FSpellExecContextV3& Ctx, const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx, const TArray<FHitResult>& Hits) const;
};

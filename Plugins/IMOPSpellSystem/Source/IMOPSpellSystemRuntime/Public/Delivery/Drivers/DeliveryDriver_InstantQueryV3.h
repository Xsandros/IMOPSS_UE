#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_InstantQueryV3.generated.h"

/**
 * InstantQuery driver (Composite-first):
 * - Executes one deterministic query immediately on Start (Overlap/Sweep/LineTrace)
 * - Emits debug draw only (Phase 4 scope)
 * - Writes hits into TargetStore if available (optional, uses OutTargetSetName / group default)
 * - Marks itself inactive after Start (does NOT auto-remove itself from group maps)
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_InstantQueryV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) override;

private:
	// Snapshot used for Stop logs/debug
	UPROPERTY()
	FDeliveryContextV3 LocalCtx;

	// Helpers
	static void SortHitsDeterministic(TArray<FHitResult>& Hits, const FVector& From);
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	bool DoOverlapQuery(UWorld* World, const FVector& Origin, const FQuat& Rot, const FDeliveryShapeV3& Shape, const FName& Profile, const FCollisionQueryParams& Params, TArray<FHitResult>& OutHits) const;
	bool DoSweepQuery(UWorld* World, const FVector& From, const FVector& To, const FQuat& Rot, const FDeliveryShapeV3& Shape, const FName& Profile, const FCollisionQueryParams& Params, TArray<FHitResult>& OutHits) const;
	bool DoLineTraceQuery(UWorld* World, const FVector& From, const FVector& To, const FName& Profile, const FCollisionQueryParams& Params, TArray<FHitResult>& OutHits) const;
};

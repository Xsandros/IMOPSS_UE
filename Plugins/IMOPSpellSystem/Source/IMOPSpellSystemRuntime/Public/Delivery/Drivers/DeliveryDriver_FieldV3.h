#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_FieldV3.generated.h"

/**
 * Field driver (Composite-first):
 * - Periodically evaluates an overlap (recommended) at PrimitiveCtx.FinalPoseWS.
 * - Maintains a small membership cache (enter/exit) for future event emission.
 * - DrawDebug only for now.
 * - Optional: writes current membership into TargetStore (OutTargetSetName / group default).
 *
 * NOTE: Stop is explicit (or via subsystem Stop calls). StopPolicy enforcement will come later at group level.
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_FieldV3 : public UDeliveryDriverBaseV3
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

	// Membership cache (deterministic-ish: server authoritative; stable for game logic)
	UPROPERTY()
	TSet<TWeakObjectPtr<AActor>> CurrentMembers;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void SortActorsDeterministic(TArray<AActor*>& Actors);

	bool EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	bool DoOverlap(UWorld* World, const FVector& Origin, const FQuat& Rot, const struct FDeliveryShapeV3& Shape, FName Profile, const FCollisionQueryParams& Params, TArray<FHitResult>& OutHits) const;
};

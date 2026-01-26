#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_InstantQueryV3.generated.h"

/**
 * InstantQuery driver (Composite-first):
 * - Evaluates once on Start (no Tick).
 * - Uses PrimitiveCtx.FinalPoseWS as origin+forward.
 * - Ray => line trace; other shapes => sweep (or overlap if Spec.Query.Mode==Overlap).
 * - Writes hit actors into TargetStore (PrimitiveCtx.Spec.OutTargetSetName or Group default).
 * - DebugDraw uses Group defaults unless primitive override enabled.
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

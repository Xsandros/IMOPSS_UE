#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"

#include "DeliveryDriver_FieldV3.generated.h"

class UDeliveryGroupRuntimeV3;

/**
 * Field driver (Composite-first)
 * - Ticks at Spec.Field.TickInterval
 * - Uses PrimitiveCtx.FinalPoseWS as center (already includes AnchorRef)
 * - Overlap sphere/box/capsule based on Spec.Shape (default sphere)
 * - Optional Enter/Exit events (Spec.Field.bEmitEnterExit)
 * - Emits Stay every evaluation if there are occupants
 * - Writes occupants into TargetStore (Primitive OutTargetSetName or Group default)
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
	float TimeSinceLastEval = 0.f;

	// Current occupants of the field (weak pointers, no ownership)
	TSet<TWeakObjectPtr<AActor>> CurrentSet;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);
	static void BuildSortedActorsDeterministic(const TSet<TWeakObjectPtr<AActor>>& InSet, TArray<AActor*>& OutActors);

	void Evaluate(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group);
};

#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_FieldV3.generated.h"

/**
 * Field driver (Composite-first):
 * - Periodically evaluates an overlap/sweep volume at PrimitiveCtx.FinalPoseWS.
 * - Writes current members into TargetStore (OutTargetSetName / group default).
 * - Optional enter/exit bookkeeping (for later events); currently only logs when enabled.
 * - Debug draw volume + member points.
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

	// last evaluated members (raw pointers for speed; lifetime guarded by world checks)
	TSet<TWeakObjectPtr<AActor>> LastMembers;

private:
	static FName ResolveOutTargetSetName(const UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	bool EvaluateOnce(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx);

	static void AddHitActorsUnique(const TArray<FHitResult>& Hits, TArray<TWeakObjectPtr<AActor>>& OutActors);
};

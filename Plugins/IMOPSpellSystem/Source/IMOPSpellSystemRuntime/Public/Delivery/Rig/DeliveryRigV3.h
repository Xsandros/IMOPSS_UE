#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/Rig/DeliveryRigNodeV3.h"

#include "DeliveryRigV3.generated.h"

/**
 * DeliveryRigV3
 * - Small deterministic rig graph that produces:
 *   - Root local-space transform (LS)
 *   - N emitter local-space transforms (LS), relative to Root
 *
 * Evaluated by DeliveryRigEvaluatorV3 which turns LS into WS based on Attach.
 *
 * NOTE: This is "data-only + eval" and does not spawn actors/components.
 */
UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigV3 : public UObject
{
	GENERATED_BODY()

public:
	// Root node: defines root motion/pose in local space.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TObjectPtr<UDeliveryRigNodeV3> RootNode = nullptr;

	// Emitter nodes: each defines an emitter pose in local space relative to root.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<TObjectPtr<UDeliveryRigNodeV3>> EmitterNodes;

public:
	/**
	 * Evaluate at time t (seconds since group start).
	 * Outputs LOCAL SPACE transforms.
	 */
	UFUNCTION(BlueprintCallable, Category="Delivery|Rig")
	void Evaluate(float TimeSeconds, FTransform& OutRootLS, TArray<FTransform>& OutEmittersLS) const;

private:
	// Helper: safe node eval (null => identity)
	static FTransform EvalNode(const UDeliveryRigNodeV3* Node, float TimeSeconds);
};

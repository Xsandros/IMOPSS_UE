#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/Rig/DeliveryRigNodeV3.h"

#include "DeliveryRigV3.generated.h"

/**
 * DeliveryRigV3
 * - Deterministic rig graph that produces:
 *   - Root local-space transform (LS)
 *   - N emitter local-space transforms (LS), relative to Root
 *
 * Evaluated by UDeliveryRigEvaluatorV3 which turns LS into WS based on Attach.
 *
 * Contract stability goal:
 * - RootNode + EmitterNodes must be enough for Phase 4.
 * - Optional naming/anchors can be added without breaking existing usage.
 */
UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigV3 : public UObject
{
	GENERATED_BODY()

public:
	/** Root node: defines root motion/pose in local space. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TObjectPtr<UDeliveryRigNodeV3> RootNode = nullptr;

	/**
	 * Emitter nodes: each defines an emitter pose in local space relative to root.
	 * Index in this array is the canonical EmitterIndex used by primitives.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<TObjectPtr<UDeliveryRigNodeV3>> EmitterNodes;

	/**
	 * Optional: names parallel to EmitterNodes (can be empty).
	 * If filled, should be same length as EmitterNodes.
	 *
	 * NOTE: Evaluator can propagate these names to results without affecting older users.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FName> EmitterNames;

public:
	/**
	 * Evaluate at time t (seconds since group start).
	 * Outputs LOCAL SPACE transforms.
	 *
	 * This signature is kept stable because evaluator/subsystem depend on it.
	 */
	UFUNCTION(BlueprintCallable, Category="Delivery|Rig")
	void Evaluate(float TimeSeconds, FTransform& OutRootLS, TArray<FTransform>& OutEmittersLS) const;

private:
	/** Helper: safe node eval (null => identity) */
	static FTransform EvalNode(const UDeliveryRigNodeV3* Node, float TimeSeconds);
};

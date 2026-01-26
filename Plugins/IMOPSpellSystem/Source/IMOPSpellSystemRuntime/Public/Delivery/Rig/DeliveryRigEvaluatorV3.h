#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/Rig/DeliveryRigV3.h"
#include "Delivery/DeliveryTypesV3.h"

#include "DeliveryRigEvaluatorV3.generated.h"

/**
 * Result of evaluating a DeliveryRig at a given time.
 * World-space transforms (WS).
 *
 * Contract:
 * - RootWorld is the group root in world space after Attach is applied.
 * - EmittersWorld are world-space transforms (EmitterLS * RootWorld).
 * - Optional: EmitterNames is parallel to EmittersWorld (same length), may be empty.
 * - Optional: AnchorsWorld provides named anchors in world-space (can be empty).
 */
USTRUCT(BlueprintType)
struct FDeliveryRigEvalResultV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FTransform RootWorld = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FTransform> EmittersWorld;

	// Optional parallel array (can be empty). If filled, must match EmittersWorld.Num().
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FName> EmitterNames;

	// Optional named anchors (can be empty).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TMap<FName, FTransform> AnchorsWorld;
};

/**
 * Stateless evaluator: given a Rig graph + attach mode, produce Root + Emitters world transforms.
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigEvaluatorV3 : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Evaluate the rig at time t (seconds since group start).
	 */
	static void Evaluate(
		UWorld* World,
		AActor* Caster,
		const FDeliveryAttachV3& Attach,
		const UDeliveryRigV3* Rig,
		float TimeSeconds,
		FDeliveryRigEvalResultV3& OutResult
	);
};

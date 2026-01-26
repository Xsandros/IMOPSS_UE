#pragma once

#include "CoreMinimal.h"
#include "Delivery/Rig/DeliveryRigV3.h"
#include "Delivery/DeliveryTypesV3.h"

#include "DeliveryRigEvaluatorV3.generated.h"

/**
 * Result of evaluating a DeliveryRig at a given time.
 * World-space transforms (WS).
 */
USTRUCT(BlueprintType)
struct FDeliveryRigEvalResultV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FTransform RootWorld = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FTransform> EmittersWorld;
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

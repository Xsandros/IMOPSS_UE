#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Delivery/DeliveryContextV3.h"
#include "Delivery/Rig/DeliveryRigV3.h"
#include "DeliveryRigEvaluatorV3.generated.h"

USTRUCT(BlueprintType)
struct FDeliveryRigPoseV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FRotator Rotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FDeliveryRigEvalResultV3
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	FDeliveryRigPoseV3 Root;

	// Optional multi-emitter poses (e.g., OrbitSampler)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Delivery|Rig")
	TArray<FDeliveryRigPoseV3> Emitters;
};

struct IMOPSPELLSYSTEMRUNTIME_API FDeliveryRigEvaluatorV3
{
	static bool Evaluate(
		const FSpellExecContextV3& Ctx,
		const FDeliveryContextV3& DeliveryCtx,
		const FDeliveryRigV3& Rig,
		FDeliveryRigEvalResultV3& OutResult);
};

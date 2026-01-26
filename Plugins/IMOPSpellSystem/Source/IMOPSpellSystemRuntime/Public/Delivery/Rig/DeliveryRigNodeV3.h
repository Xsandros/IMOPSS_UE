#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DeliveryRigNodeV3.generated.h"

/**
 * Rig Node base:
 * Evaluates a LOCAL SPACE transform at time t.
 *
 * Determinism rules:
 * - No random calls here (seeded noise can be implemented later via blackboard-driven inputs)
 * - Pure function of (TimeSeconds + node parameters)
 */
UCLASS(Abstract, BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNodeV3 : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Delivery|Rig")
	FTransform Evaluate(float TimeSeconds) const;
	virtual FTransform Evaluate_Implementation(float TimeSeconds) const { return FTransform::Identity; }
};

// ============================================================
// Standard nodes (small, useful, deterministic)
// ============================================================

/** Constant transform */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNode_StaticPoseV3 : public UDeliveryRigNodeV3
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	FTransform LocalPose = FTransform::Identity;

	virtual FTransform Evaluate_Implementation(float TimeSeconds) const override
	{
		return LocalPose;
	}
};

/**
 * Sine offset along a local axis.
 * Output = BasePose * (translation axis * sin(t*Freq + Phase)*Amplitude)
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNode_SineOffsetV3 : public UDeliveryRigNodeV3
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	FTransform BasePose = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	FVector LocalAxis = FVector(0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float Amplitude = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float Frequency = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float Phase = 0.f;

	virtual FTransform Evaluate_Implementation(float TimeSeconds) const override
	{
		const float W = TimeSeconds * Frequency * 2.f * PI + Phase;
		const float S = FMath::Sin(W);

		FTransform Offset = FTransform::Identity;
		Offset.SetLocation(LocalAxis.GetSafeNormal() * (Amplitude * S));
		return Offset * BasePose;
	}
};

/**
 * Spin around local axis (yaw-like rotation by default).
 * Output = BasePose * Rot(axis, degrees = SpeedDegPerSec * t + OffsetDeg)
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNode_SpinV3 : public UDeliveryRigNodeV3
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	FTransform BasePose = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	FVector LocalAxis = FVector(0.f, 0.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float SpeedDegPerSec = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float OffsetDeg = 0.f;

	virtual FTransform Evaluate_Implementation(float TimeSeconds) const override
	{
		const float Deg = OffsetDeg + SpeedDegPerSec * TimeSeconds;
		const FVector Axis = LocalAxis.GetSafeNormal();
		const FQuat Q(Axis, FMath::DegreesToRadians(Deg));

		FTransform R = FTransform::Identity;
		R.SetRotation(Q);
		return R * BasePose;
	}
};

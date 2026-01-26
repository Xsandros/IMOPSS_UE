#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DeliveryRigNodeV3.generated.h"

/**
 * Rig Node base:
 * Evaluates a LOCAL SPACE transform at time t.
 *
 * Determinism rules:
 * - No unseeded randomness here.
 * - Pure function of (TimeSeconds + node parameters).
 *
 * Composition rule (important):
 * - Nodes should generally return "Delta * BasePose" (or "BasePose then Delta"), but stay consistent per node.
 * - We keep everything as transforms so the evaluator can remain stateless.
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
 * Output = Offset * BasePose
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
 * Spin around local axis.
 * Output = Rot * BasePose
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

		FTransform R = FTransform::Identity;
		R.SetRotation(FQuat(Axis, FMath::DegreesToRadians(Deg)));

		return R * BasePose;
	}
};

// ============================================================
// Composition nodes (to make rigs expressive without special-casing)
// ============================================================

/**
 * Compose(A, B): Output = A(Time) * B(Time)
 * Useful for chaining deltas or building "helix around beam" style motion.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNode_ComposeV3 : public UDeliveryRigNodeV3
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	TObjectPtr<UDeliveryRigNodeV3> A = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	TObjectPtr<UDeliveryRigNodeV3> B = nullptr;

	virtual FTransform Evaluate_Implementation(float TimeSeconds) const override
	{
		const FTransform TA = A ? A->Evaluate(TimeSeconds) : FTransform::Identity;
		const FTransform TB = B ? B->Evaluate(TimeSeconds) : FTransform::Identity;
		return TA * TB;
	}
};

/**
 * Lerp(A, B, Alpha): Alpha can be constant or driven by time via AlphaFrequency.
 * Output blends translation+rotation+scale in a deterministic way.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryRigNode_LerpV3 : public UDeliveryRigNodeV3
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	TObjectPtr<UDeliveryRigNodeV3> A = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	TObjectPtr<UDeliveryRigNodeV3> B = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float Alpha = 0.5f;

	/** Optional: if >0, Alpha becomes 0.5+0.5*sin(t*freq) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rig")
	float AlphaFrequency = 0.f;

	virtual FTransform Evaluate_Implementation(float TimeSeconds) const override
	{
		float T = Alpha;
		if (AlphaFrequency > 0.f)
		{
			T = 0.5f + 0.5f * FMath::Sin(TimeSeconds * AlphaFrequency * 2.f * PI);
		}
		T = FMath::Clamp(T, 0.f, 1.f);

		const FTransform TA = A ? A->Evaluate(TimeSeconds) : FTransform::Identity;
		const FTransform TB = B ? B->Evaluate(TimeSeconds) : FTransform::Identity;

		// Deterministic transform blend
		const FVector L = FMath::Lerp(TA.GetLocation(), TB.GetLocation(), T);
		const FQuat   Q = FQuat::Slerp(TA.GetRotation(), TB.GetRotation(), T).GetNormalized();
		const FVector S = FMath::Lerp(TA.GetScale3D(),  TB.GetScale3D(),  T);

		return FTransform(Q, L, S);
	}
};

#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_MoverV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_MoverV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) override;

	virtual FDeliveryHandleV3 GetHandle() const override { return DeliveryCtx.Handle; }
	virtual bool IsActive() const override { return bActive; }

private:
	FDeliveryContextV3 DeliveryCtx;
	bool bActive = false;

	FVector Position = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;

	float DistanceTraveled = 0.f;
	float TimeSinceStart = 0.f;
	float TimeSinceLastEval = 0.f;

	int32 PierceHitsSoFar = 0;
	TSet<TWeakObjectPtr<AActor>> UniqueHitTargets;

	// helpers
	FTransform ResolveAttachTransform(const FSpellExecContextV3& Ctx) const;

	// motion
	void IntegrateMotion(const FSpellExecContextV3& Ctx, float Dt);

	// collision
	void EvaluateSweep(const FSpellExecContextV3& Ctx, const FVector& From, const FVector& To);

	// homing
	AActor* ChooseHomingTargetDeterministic(const FSpellExecContextV3& Ctx) const;
};

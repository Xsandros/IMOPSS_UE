#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_FieldV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_FieldV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx) override;
	virtual void Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) override;

	virtual FDeliveryHandleV3 GetHandle() const override { return DeliveryCtx.Handle; }
	virtual bool IsActive() const override { return bActive; }

private:
	// ==== Runtime State ====
	FDeliveryContextV3 DeliveryCtx;
	bool bActive = false;

	float TimeSinceLastEval = 0.f;

	// current occupants
	TSet<TWeakObjectPtr<AActor>> CurrentSet;

	// helpers
	FTransform ResolveAttachTransform(const FSpellExecContextV3& Ctx) const;
	void Evaluate(const FSpellExecContextV3& Ctx);
};

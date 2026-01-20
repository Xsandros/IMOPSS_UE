#pragma once

#include "CoreMinimal.h"
#include "Delivery/Drivers/DeliveryDriverBaseV3.h"
#include "DeliveryDriver_InstantQueryV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriver_InstantQueryV3 : public UDeliveryDriverBaseV3
{
	GENERATED_BODY()
public:
	virtual void Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx) override;
	virtual void Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) override;

	virtual FDeliveryHandleV3 GetHandle() const override { return DeliveryCtx.Handle; }
	virtual bool IsActive() const override { return bActive; }

private:
	FDeliveryContextV3 DeliveryCtx;
	bool bActive = false;

	FTransform ResolveAttachTransform(const FSpellExecContextV3& Ctx) const;
};

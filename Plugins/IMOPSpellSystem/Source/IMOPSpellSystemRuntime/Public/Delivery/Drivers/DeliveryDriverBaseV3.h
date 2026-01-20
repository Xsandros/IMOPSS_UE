#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Delivery/DeliveryContextV3.h"
#include "DeliveryDriverBaseV3.generated.h"

struct FSpellExecContextV3;

UCLASS(Abstract)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriverBaseV3 : public UObject
{
	GENERATED_BODY()
public:
	virtual void Start(const FSpellExecContextV3& Ctx, const FDeliveryContextV3& InDeliveryCtx) PURE_VIRTUAL(UDeliveryDriverBaseV3::Start, );
	virtual void Tick(const FSpellExecContextV3& Ctx, float DeltaSeconds) {}
	virtual void Stop(const FSpellExecContextV3& Ctx, EDeliveryStopReasonV3 Reason) PURE_VIRTUAL(UDeliveryDriverBaseV3::Stop, );

	virtual FDeliveryHandleV3 GetHandle() const PURE_VIRTUAL(UDeliveryDriverBaseV3::GetHandle, return FDeliveryHandleV3(););
	virtual bool IsActive() const PURE_VIRTUAL(UDeliveryDriverBaseV3::IsActive, return false;);
};

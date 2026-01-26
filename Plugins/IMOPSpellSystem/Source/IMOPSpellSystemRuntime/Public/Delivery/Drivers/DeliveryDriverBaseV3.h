#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Delivery/DeliveryTypesV3.h"
#include "Delivery/DeliveryContextV3.h"

#include "DeliveryDriverBaseV3.generated.h"

struct FSpellExecContextV3;
class UDeliveryGroupRuntimeV3;

UCLASS(Abstract, BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UDeliveryDriverBaseV3 : public UObject
{
	GENERATED_BODY()

public:
	virtual void Start(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, const FDeliveryContextV3& PrimitiveCtx) PURE_VIRTUAL(UDeliveryDriverBaseV3::Start, );
	virtual void Tick(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, float DeltaSeconds) {}
	virtual void Stop(const FSpellExecContextV3& Ctx, UDeliveryGroupRuntimeV3* Group, EDeliveryStopReasonV3 Reason) PURE_VIRTUAL(UDeliveryDriverBaseV3::Stop, );

	virtual bool IsActive() const { return bActive; }

	virtual FDeliveryHandleV3 GetGroupHandle() const { return GroupHandle; }
	virtual FName GetPrimitiveId() const { return PrimitiveId; }

protected:
	UPROPERTY()
	bool bActive = false;

	UPROPERTY()
	FDeliveryHandleV3 GroupHandle;

	UPROPERTY()
	FName PrimitiveId = NAME_None;
};

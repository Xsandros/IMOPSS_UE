#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Exec_StopDeliveryV3.generated.h"

/**
 * Action: Action.Delivery.Stop
 * Payload: FPayload_DeliveryStopV3
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UExec_StopDeliveryV3 : public USpellActionExecutorV3
{
	GENERATED_BODY()

public:
	virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) override;
};

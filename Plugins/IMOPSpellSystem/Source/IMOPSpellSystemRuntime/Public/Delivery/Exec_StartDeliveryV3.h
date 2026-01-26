#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Exec_StartDeliveryV3.generated.h"

/**
 * Action: Action.Delivery.Start
 * Payload: FPayload_DeliveryStartV3
 */
UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UExec_StartDeliveryV3 : public USpellActionExecutorV3
{
	GENERATED_BODY()

public:
	virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) override;
};

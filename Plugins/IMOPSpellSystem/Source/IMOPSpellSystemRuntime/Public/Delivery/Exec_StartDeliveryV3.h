#pragma once

#include "CoreMinimal.h"
#include "SpellPayloadsDeliveryV3.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Exec_StartDeliveryV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UExec_StartDeliveryV3 : public USpellActionExecutorV3
{
	GENERATED_BODY()
public:
	virtual const UScriptStruct* GetPayloadStruct() const override { return FPayload_DeliveryStartV3::StaticStruct(); }
	virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) override;
};

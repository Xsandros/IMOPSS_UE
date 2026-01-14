#pragma once

#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Exec_ReadAttributeV3.generated.h"

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UExec_ReadAttributeV3 : public USpellActionExecutorV3
{
	GENERATED_BODY()
public:
	virtual const UScriptStruct* GetPayloadStruct() const override;
	virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) override;
};

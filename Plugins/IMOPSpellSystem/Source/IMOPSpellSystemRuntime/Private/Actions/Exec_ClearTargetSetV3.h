#pragma once
#include "CoreMinimal.h"
#include "Actions/SpellActionExecutorV3.h"
#include "Exec_ClearTargetSetV3.generated.h"

UCLASS()
class UExec_ClearTargetSetV3 : public USpellActionExecutorV3
{
	GENERATED_BODY()
public:
	virtual const UScriptStruct* GetPayloadStruct() const override;
	virtual void Execute(const FSpellExecContextV3& Ctx, const void* PayloadData) override;
};

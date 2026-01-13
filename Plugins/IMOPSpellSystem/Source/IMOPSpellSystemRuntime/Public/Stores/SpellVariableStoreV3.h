#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Actions/SpellPayloadsCoreV3.h"
#include "SpellVariableStoreV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellVariableStoreV3 : public UObject
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Spell|Vars")
    void SetValue(FName Key, const FSpellValueV3& Value);

    UFUNCTION(BlueprintCallable, Category = "Spell|Vars")
    bool GetValue(FName Key, FSpellValueV3& OutValue) const;

    // Numeric modify helper (float)
    bool ModifyFloat(FName Key, ESpellNumericOpV3 Op, float Operand);

private:
    UPROPERTY()
    TMap<FName, FSpellValueV3> Values;
};

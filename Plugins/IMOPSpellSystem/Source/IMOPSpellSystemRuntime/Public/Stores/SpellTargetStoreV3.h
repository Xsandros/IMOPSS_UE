#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Targeting/TargetingTypesV3.h"
#include "SpellTargetStoreV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellTargetStoreV3 : public UObject
{
    GENERATED_BODY()
public:
    // Finaler Contract: Find/Get/Set/Clear
    const FTargetSetV3* Find(FName Key) const;

    bool Get(FName Key, FTargetSetV3& Out) const;

    void Set(FName Key, const FTargetSetV3& Set);

    void Clear(FName Key);

private:
    UPROPERTY()
    TMap<FName, FTargetSetV3> Sets;
};

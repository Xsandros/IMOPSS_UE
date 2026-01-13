#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Targeting/TargetingBackendsV3.h"
#include "TargetingBackend_DefaultV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UTargetingBackend_DefaultV3
    : public UObject
    , public ITargetingBackendV3
{
    GENERATED_BODY()
public:
    virtual bool RadiusQuery(
        const UWorld* World,
        const FVector& Origin,
        float Radius,
        FTargetQueryResultV3& OutResult,
        FText* OutError = nullptr) const override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Targeting")
    TEnumAsByte<ECollisionChannel> OverlapChannel = ECC_Pawn;
};

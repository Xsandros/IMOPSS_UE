#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Targeting/TargetingBackendsV3.h"
#include "TargetingBackend_DefaultV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UTargetingBackend_DefaultV3 : public UObject, public ITargetingBackendV3
{
    GENERATED_BODY()
public:
    UTargetingBackend_DefaultV3();

    virtual bool RadiusQuery(
        const UWorld* World,
        const FVector& Origin,
        float Radius,
        FTargetQueryResultV3& OutResult,
        FText* OutError = nullptr) const override;
    
    // Preferred: allow multiple object types (Pawn + WorldDynamic, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Targeting")
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;

    // Legacy fallback if ObjectTypes is empty
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spell|Targeting")
    TEnumAsByte<ECollisionChannel> OverlapChannel = ECC_Pawn;
};


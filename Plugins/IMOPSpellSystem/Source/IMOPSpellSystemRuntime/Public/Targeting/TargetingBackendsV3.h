#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Targeting/TargetingTypesV3.h"
#include "TargetingBackendsV3.generated.h"

USTRUCT(BlueprintType)
struct FTargetQueryResultV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTargetRefV3> Candidates;
};

UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UTargetingBackendV3 : public UInterface
{
    GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API ITargetingBackendV3
{
    GENERATED_BODY()
public:
    virtual bool RadiusQuery(
        const UWorld* World,
        const FVector& Origin,
        float Radius,
        FTargetQueryResultV3& OutResult,
        FText* OutError = nullptr) const = 0;
};

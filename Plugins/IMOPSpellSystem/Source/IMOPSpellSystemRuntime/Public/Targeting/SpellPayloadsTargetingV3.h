#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingTypesV3.h"
#include "Targeting/TargetingSubsystemV3.h"
#include "SpellPayloadsTargetingV3.generated.h"

USTRUCT(BlueprintType)
struct FPayload_TargetAcquireV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OutTargetSet = "Targets";
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetAcquireRequestV3 Request;
};

USTRUCT(BlueprintType)
struct FPayload_TargetFilterV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName InTargetSet = "Targets";
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OutTargetSet = "TargetsFiltered";
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetOriginV3 Origin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTargetFilterV3> Filters;
};

USTRUCT(BlueprintType)
struct FPayload_TargetSelectV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName InTargetSet = "TargetsFiltered";
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName OutTargetSet = "TargetsSelected";
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetOriginV3 Origin;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetSelectV3 Selection;
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Targeting/TargetingTypesV3.h"
#include "TargetingSubsystemV3.generated.h"

struct FSpellExecContextV3;

USTRUCT(BlueprintType)
struct FTargetAcquireRequestV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) ETargetAcquireKindV3 Kind = ETargetAcquireKindV3::RadiusQuery;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetOriginV3 Origin;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float Radius = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName SourceTargetSet = NAME_None;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<TObjectPtr<AActor>> ExplicitActors;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTargetFilterV3> Filters;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FTargetSelectV3 Select;
};

USTRUCT(BlueprintType)
struct FTargetAcquireResponseV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTargetRefV3> Candidates;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FTargetRefV3> Selected;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bSucceeded = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FText Error;
};

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API UTargetingSubsystemV3 : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    // Backend/Hooks injection (final-form)
    void SetBackendObject(UObject* InBackend);
    void SetGameHooksObject(UObject* InHooks);

    UObject* GetBackendObject() const { return BackendObject; }
    UObject* GetGameHooksObject() const { return GameHooksObject; }

    bool AcquireTargets(
        const FSpellExecContextV3& Ctx,
        const FTargetAcquireRequestV3& Request,
        FTargetAcquireResponseV3& OutResponse) const;

private:
    UPROPERTY() TObjectPtr<UObject> BackendObject;
    UPROPERTY() TObjectPtr<UObject> GameHooksObject;
};

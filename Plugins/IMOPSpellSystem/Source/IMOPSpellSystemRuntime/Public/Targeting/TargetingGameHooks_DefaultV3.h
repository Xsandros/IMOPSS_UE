#pragma once

#include "CoreMinimal.h"
#include "Targeting/TargetingGameHooksV3.h"
#include "TargetingGameHooks_DefaultV3.generated.h"

UCLASS(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API UTargetingGameHooks_DefaultV3 : public UObject, public ITargetingGameHooksV3
{
    GENERATED_BODY()

public:
    // Minimal: nur Self/Any. Ally/Enemy kommt später game-spezifisch.
    virtual ETargetRelationV3 GetRelation(const FSpellExecContextV3& Ctx, const AActor* Candidate) const override;

    // Unterstützt IGameplayTagAssetInterface falls vorhanden
    virtual bool HasGameplayTag(const AActor* Candidate, FGameplayTag Tag) const override;

    // Phase 2 default: false (später an Status-System ankoppeln)
    virtual bool HasStatusTag(const AActor* Candidate, FGameplayTag StatusTag) const override;

    // Simple visibility trace
    virtual bool HasLineOfSight(const FSpellExecContextV3& Ctx, const FVector& Origin, const AActor* Candidate) const override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Targeting|LOS")
    TEnumAsByte<ECollisionChannel> LineTraceChannel = ECC_Visibility;
};

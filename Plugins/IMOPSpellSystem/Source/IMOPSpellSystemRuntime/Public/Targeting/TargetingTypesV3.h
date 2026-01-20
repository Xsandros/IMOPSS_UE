#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "TargetingTypesV3.generated.h"

UENUM(BlueprintType)
enum class ETargetRelationV3 : uint8
{
    Any,
    Self,
    Ally,
    Enemy,
    Neutral
};

UENUM(BlueprintType)
enum class ETargetOriginKindV3 : uint8
{
    Caster,
    CasterView,        // <-- hinzufügen
    Actor,
    WorldLocation,
    TargetSetCenter
};


USTRUCT(BlueprintType)
struct FTargetRefV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TWeakObjectPtr<AActor> Actor;

    bool IsValid() const { return Actor.IsValid(); }

    // Optional aber sehr hilfreich für deterministische Tie-Breaks
    int32 StableId() const { return Actor.IsValid() ? Actor->GetUniqueID() : 0; }
};

USTRUCT(BlueprintType)
struct FTargetSetV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FTargetRefV3> Targets;

    void Reset() { Targets.Reset(); }
    void RemoveInvalid() { Targets.RemoveAll([](const FTargetRefV3& R){ return !R.IsValid(); }); }

    void AddUnique(const FTargetRefV3& Ref)
    {
        if (!Ref.IsValid()) return;
        for (const FTargetRefV3& T : Targets)
        {
            if (T.Actor == Ref.Actor) return;
        }
        Targets.Add(Ref);
    }
};

// ----------------- Acquire -----------------

UENUM(BlueprintType)
enum class ETargetAcquireKindV3 : uint8
{
    ExplicitActors,
    FromTargetSet,
    RadiusQuery
};

USTRUCT(BlueprintType)
struct FTargetOriginV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) ETargetOriginKindV3 Kind = ETargetOriginKindV3::Caster;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TWeakObjectPtr<AActor> Actor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FVector WorldLocation = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName TargetSetKey = NAME_None;
};

// ----------------- Filters -----------------

UENUM(BlueprintType)
enum class ETargetFilterOpV3 : uint8
{
    Keep,
    Drop
};

UENUM(BlueprintType)
enum class ETargetFilterKindV3 : uint8
{
    Relation,

    // Single-tag (backwards compatible)
    HasTag,

    // Tag containers (final)
    HasAnyTags,
    HasAllTags,
    HasNoneTags,

    // Status (single tag)
    HasStatus,

    LineOfSight,
    DistanceRange,

    // Components (final)
    HasComponent,
    HasAttributeComponent,
    HasStatusComponent
};


USTRUCT(BlueprintType)
struct FTargetFilterV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETargetFilterKindV3 Kind = ETargetFilterKindV3::Relation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETargetFilterOpV3 Op = ETargetFilterOpV3::Keep;

    // --- Relation ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ETargetRelationV3 Relation = ETargetRelationV3::Any;

    // --- Single tag (legacy / quick) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag Tag;

    // --- Tag containers (final) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer Tags;

    // --- Distance ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MinDistance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float MaxDistance = 0.f;

    // --- Components ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UActorComponent> ComponentClass;
};


// ----------------- Selection -----------------

UENUM(BlueprintType)
enum class ETargetSelectKindV3 : uint8
{
    All,
    Nearest,
    Farthest,
    Random
};

USTRUCT(BlueprintType)
struct FTargetSelectV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite) ETargetSelectKindV3 Kind = ETargetSelectKindV3::All;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 Count = 0; // 0 = all
    UPROPERTY(EditAnywhere, BlueprintReadWrite) bool bUnique = true;
};

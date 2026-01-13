#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InstancedStruct.h"
#include "StructUtils/InstancedStruct.h"
#include "SpellActionV3.generated.h"

UENUM(BlueprintType)
enum class ESpellActionPolicyV3 : uint8
{
    None = 0,
    RequireAuthority = 1 << 0
};

ENUM_CLASS_FLAGS(ESpellActionPolicyV3)

USTRUCT(BlueprintType)
struct FSpellActionV3
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Action")
    FGameplayTag ActionTag;

    // Final-form payload container
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Action")
    FInstancedStruct Payload;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spell|Action")
    ESpellActionPolicyV3 Policy = ESpellActionPolicyV3::None;
};

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SpellTriggerMatcherV3.h"
#include "SpellEventV3.h"
#include "SpellEventBusSubsystemV3.generated.h"

USTRUCT(BlueprintType)
struct FSpellEventSubscriptionHandleV3
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Id = INDEX_NONE;

    bool IsValid() const { return Id != INDEX_NONE; }
};

// ✅ MOVE THIS OUTSIDE THE CLASS (global scope)
USTRUCT()
struct FSpellEventBusSubscriberV3
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Id = INDEX_NONE;

    UPROPERTY()
    TWeakObjectPtr<UObject> Listener;

    UPROPERTY()
    FSpellTriggerMatcherV3 Matcher;
};

UCLASS()
class IMOPSPELLSYSTEMRUNTIME_API USpellEventBusSubsystemV3 : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    FSpellEventSubscriptionHandleV3 Subscribe(UObject* Listener, const FSpellTriggerMatcherV3& Matcher);
    bool Unsubscribe(const FSpellEventSubscriptionHandleV3& Handle);
    void Emit(const FSpellEventV3& Ev);

private:
    int32 NextId = 1;

    UPROPERTY()
    TArray<FSpellEventBusSubscriberV3> Subscribers;
};

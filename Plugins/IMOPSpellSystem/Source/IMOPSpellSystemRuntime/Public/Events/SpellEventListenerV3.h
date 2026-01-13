#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SpellEventListenerV3.generated.h"

struct FSpellEventV3;

UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellEventListenerV3 : public UInterface
{
    GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API ISpellEventListenerV3
{
    GENERATED_BODY()

public:
    // Called when an event matches the listener subscription.
    virtual void OnSpellEvent(const FSpellEventV3& Ev) = 0;
};

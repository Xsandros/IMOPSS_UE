#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Events/SpellEventV3.h"
#include "SpellEventListenerV3.generated.h"

/**
 * Listener interface for SpellEventBusSubsystemV3.
 */
UINTERFACE(BlueprintType)
class IMOPSPELLSYSTEMRUNTIME_API USpellEventListenerV3 : public UInterface
{
    GENERATED_BODY()
};

class IMOPSPELLSYSTEMRUNTIME_API ISpellEventListenerV3
{
    GENERATED_BODY()

public:
    // Called when an event is emitted and matches subscription filter.
    virtual void OnSpellEvent(const FSpellEventV3& Ev) = 0;
};

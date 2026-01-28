#include "Spec/SpellSpecV3.h"
#if WITH_EDITOR
#include "UObject/UnrealType.h"
#endif

// include payload structs you want to auto-init:
#include "Delivery/SpellPayloadsDeliveryV3.h"
#include "Actions/SpellPayloadsCoreV3.h"

#if WITH_EDITOR
static const UScriptStruct* ResolveDefaultPayloadStruct(const FGameplayTag& ActionTag)
{
    // NOTE: Replace these tag lookups with your SpellGameplayTagsV3 accessors if you have them.
    // Using string tags is okay in editor code, but native tags are better.

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.Delivery.Start"))))
    {
        return FPayload_DeliveryStartV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.Delivery.Stop"))))
    {
        return FPayload_DeliveryStopV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.EmitEvent"))))
    {
        return FPayload_EmitEventV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.SetVariable"))))
    {
        return FPayload_SetVariableV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.ModifyVariable"))))
    {
        return FPayload_ModifyVariableV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.SetTargetSet"))))
    {
        return FPayload_SetTargetSetV3::StaticStruct();
    }

    if (ActionTag.MatchesTagExact(FGameplayTag::RequestGameplayTag(FName("Spell.Action.ClearTargetSet"))))
    {
        return FPayload_ClearTargetSetV3::StaticStruct();
    }

    return nullptr;
}

void USpellSpecV3::PostEditChangeProperty(FPropertyChangedEvent& E)
{
    Super::PostEditChangeProperty(E);

    // Re-scan all actions; cheap enough for editor and avoids needing deep property path logic.
    for (FSpellPhaseV3& Phase : Phases)
    {
        for (FSpellActionV3& Action : Phase.Actions)
        {
            const UScriptStruct* Expected = ResolveDefaultPayloadStruct(Action.ActionTag);
            if (!Expected)
            {
                continue;
            }

            if (!Action.Payload.IsValid() || Action.Payload.GetScriptStruct() != Expected)
            {
                Action.Payload.InitializeAs(Expected);
            }
        }
    }

    for (FSpellHandlerV3& Handler : Handlers)
    {
        for (FSpellActionV3& Action : Handler.Actions)
        {
            const UScriptStruct* Expected = ResolveDefaultPayloadStruct(Action.ActionTag);
            if (!Expected)
            {
                continue;
            }

            if (!Action.Payload.IsValid() || Action.Payload.GetScriptStruct() != Expected)
            {
                Action.Payload.InitializeAs(Expected);
            }
        }
    }
}
#endif

#include "Helpers/SpellHelloSpellTestActorV3.h"

#include "Core/SpellCoreSubsystemV3.h"
#include "Actions/SpellActionRegistryV3.h"

#include "Events/SpellEventBusSubsystemV3.h"
#include "Runtime/SpellRuntimeV3.h"

#include "Spec/SpellSpecV3.h"
#include "Spec/SpellActionV3.h"

#include "Actions/SpellPayloadsCoreV3.h"
#include "InstancedStruct.h"

#include "Engine/World.h"
#include "Engine/GameInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPHelloSpellV3, Log, All);

static FInstancedStruct MakePayload(const UScriptStruct* S, const void* Src)
{
    FInstancedStruct IS;
    IS.InitializeAs(S);
    FMemory::Memcpy(IS.GetMutableMemory(), Src, S->GetStructureSize());
    return IS;
}

void ASpellHelloSpellTestActorV3::BeginPlay()
{
    Super::BeginPlay();

    UGameInstance* GI = GetGameInstance();
    if (!GI) return;

    USpellCoreSubsystemV3* Core = GI->GetSubsystem<USpellCoreSubsystemV3>();
    USpellEventBusSubsystemV3* Bus = GI->GetSubsystem<USpellEventBusSubsystemV3>();
    if (!Core || !Bus)
    {
        UE_LOG(LogIMOPHelloSpellV3, Error, TEXT("Missing CoreSubsystem or EventBusSubsystem."));
        return;
    }

    USpellActionRegistryV3* Registry = Core->GetActionRegistry();
    if (!Registry)
    {
        UE_LOG(LogIMOPHelloSpellV3, Error, TEXT("Missing ActionRegistry."));
        return;
    }

    // Build a transient spec
    USpellSpecV3* Spec = NewObject<USpellSpecV3>(this);
    Spec->SpellId = TEXT("HelloSpell");

    // Tag names: reuse your existing tags for start/end + add two test tags (create them in DefaultGameplayTags.ini if you want strict checking)
    const FGameplayTag EventStart = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.Start")));
    const FGameplayTag EventHello = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Test.Hello")));
    const FGameplayTag EventDone = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Test.Done")));

    const FGameplayTag ActionEmit = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Action.EmitEvent")));
    const FGameplayTag ActionSet = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Action.SetVariable")));
    const FGameplayTag ActionMod = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Action.ModifyVariable")));

    // Handler 1: On Start -> SetVariable(X=1) -> EmitEvent(Hello)
    {
        FSpellHandlerV3 H;
        H.HandlerId = TEXT("OnStart");

        H.Trigger.bUseExactTag = true;
        H.Trigger.ExactTag = EventStart;

        // SetVariable
        FPayload_SetVariableV3 PSet;
        PSet.Key = TEXT("X");
        PSet.Value.Type = ESpellValueTypeV3::Float;
        PSet.Value.Float = 1.f;

        FSpellActionV3 ASet;
        ASet.ActionTag = ActionSet;
        ASet.Payload = MakePayload(FPayload_SetVariableV3::StaticStruct(), &PSet);

        // Emit Hello
        FPayload_EmitEventV3 PEmit;
        PEmit.EventTag = EventHello;

        FSpellActionV3 AEmit;
        AEmit.ActionTag = ActionEmit;
        AEmit.Payload = MakePayload(FPayload_EmitEventV3::StaticStruct(), &PEmit);

        H.Actions = { ASet, AEmit };
        Spec->Handlers.Add(H);
    }

    // Handler 2: On Hello -> ModifyVariable(X += 1) -> EmitEvent(Done)
    {
        FSpellHandlerV3 H;
        H.HandlerId = TEXT("OnHello");

        H.Trigger.bUseExactTag = true;
        H.Trigger.ExactTag = EventHello;

        FPayload_ModifyVariableV3 PM;
        PM.Key = TEXT("X");
        PM.Op = ESpellNumericOpV3::Add;
        PM.Operand = 1.f;

        FSpellActionV3 AMod;
        AMod.ActionTag = ActionMod;
        AMod.Payload = MakePayload(FPayload_ModifyVariableV3::StaticStruct(), &PM);

        FPayload_EmitEventV3 PEmit;
        PEmit.EventTag = EventDone;

        FSpellActionV3 AEmit;
        AEmit.ActionTag = ActionEmit;
        AEmit.Payload = MakePayload(FPayload_EmitEventV3::StaticStruct(), &PEmit);

        H.Actions = { AMod, AEmit };
        Spec->Handlers.Add(H);
    }

    // Handler 3: On Done -> log variable X
    {
        FSpellHandlerV3 H;
        H.HandlerId = TEXT("OnDone");

        H.Trigger.bUseExactTag = true;
        H.Trigger.ExactTag = EventDone;

        // We'll just emit Spell.End to prove the chain ends.
        FPayload_EmitEventV3 PEmit;
        PEmit.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.End")));

        FSpellActionV3 AEmit;
        AEmit.ActionTag = ActionEmit;
        AEmit.Payload = MakePayload(FPayload_EmitEventV3::StaticStruct(), &PEmit);

        H.Actions = { AEmit };
        Spec->Handlers.Add(H);
    }

    // Start runtime
    USpellRuntimeV3* Runtime = NewObject<USpellRuntimeV3>(this);
    Runtime->Init(this, GetOwner(), Spec, Registry, Bus);
    Runtime->Start();

    UE_LOG(LogIMOPHelloSpellV3, Log, TEXT("HelloSpell runtime started."));
}

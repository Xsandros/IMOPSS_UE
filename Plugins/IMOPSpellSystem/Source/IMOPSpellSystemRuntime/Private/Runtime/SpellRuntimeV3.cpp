#include "Runtime/SpellRuntimeV3.h"

#include "Stores/SpellVariableStoreV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "VM/SpellVMV3.h"

#include "Spec/SpellSpecV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPSpellRuntimeV3, Log, All);

void USpellRuntimeV3::Init(UObject* WorldContextObj, AActor* InCaster, USpellSpecV3* InSpec, USpellActionRegistryV3* InRegistry, USpellEventBusSubsystemV3* InEventBus)
{
    WorldContext = WorldContextObj;
    Caster = InCaster;
    Spec = InSpec;
    Registry = InRegistry;
    EventBus = InEventBus;

    VariableStore = NewObject<USpellVariableStoreV3>(this);
    TargetStore = NewObject<USpellTargetStoreV3>(this);
}

void USpellRuntimeV3::Start()
{
    if (bRunning) return;
    if (!Spec || !EventBus || !Registry)
    {
        UE_LOG(LogIMOPSpellRuntimeV3, Error, TEXT("Start failed: missing Spec/EventBus/Registry."));
        return;
    }

    InitializeRng(Seed);
    bRunning = true;

    // Subscribe all handlers
    for (const FSpellHandlerV3& H : Spec->Handlers)
    {
        Subscriptions.Add(EventBus->Subscribe(this, H.Trigger));
    }

    // Subscribe phases too (phases are �event triggers to actions�)
    for (const FSpellPhaseV3& P : Spec->Phases)
    {
        Subscriptions.Add(EventBus->Subscribe(this, P.Trigger));
    }

    // Emit Spell.Start (optional convention)
    FSpellEventV3 Ev;
    Ev.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.Start")));
    Ev.Caster = Caster;
    Ev.Sender = this;
    if (UWorld* W = GetWorld())
    {
        Ev.FrameNumber = (int32)GFrameCounter;
        Ev.TimeSeconds = (float)W->GetTimeSeconds();
    }
    EventBus->Emit(Ev);
    UE_LOG(LogTemp, Log, TEXT("Runtime: Start %s"), *GetName());
}

void USpellRuntimeV3::Stop()
{
    if (!bRunning) return;
    bRunning = false;

    // Unsubscribe from EventBus
    if (EventBus)
    {
        for (const FSpellEventSubscriptionHandleV3& H : Subscriptions)
        {
            EventBus->Unsubscribe(H); // <-- muss es geben; sonst siehe Hinweis unten
        }
    }
    Subscriptions.Reset();

    UE_LOG(LogTemp, Log, TEXT("SpellRuntimeV3: Stopped and unsubscribed."));
    // Emit Spell.End (optional)
    if (EventBus)
    {
        FSpellEventV3 Ev;
        Ev.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.End")));
        Ev.Caster = Caster;
        Ev.Sender = this;
        if (UWorld* W = GetWorld())
        {
            Ev.FrameNumber = (int32)GFrameCounter;
            Ev.TimeSeconds = (float)W->GetTimeSeconds();
        }
        EventBus->Emit(Ev);
    }
    UE_LOG(LogTemp, Log, TEXT("Runtime: Stop %s"), *GetName());
}

void USpellRuntimeV3::OnSpellEvent(const FSpellEventV3& Ev)
{
    if (!bRunning || !Spec) return;

    // Dispatch handlers that match
    for (const FSpellHandlerV3& H : Spec->Handlers)
    {
        if (H.Trigger.Matches(Ev.EventTag))
        {
            FSpellVMV3::ExecuteActions(*this, H.Actions);
        }
    }

    // Dispatch phases that match
    for (const FSpellPhaseV3& P : Spec->Phases)
    {
        if (P.Trigger.Matches(Ev.EventTag))
        {
            FSpellVMV3::ExecuteActions(*this, P.Actions);
        }
    }
}

USpellActionExecutorV3* USpellRuntimeV3::GetOrCreateExecutor(TSubclassOf<USpellActionExecutorV3> ExecClass)
{
    if (!ExecClass) return nullptr;

    if (TObjectPtr<USpellActionExecutorV3>* Found = ExecutorCache.Find(ExecClass))
    {
        return Found->Get();
    }

    USpellActionExecutorV3* NewExec = NewObject<USpellActionExecutorV3>(this, ExecClass);
    ExecutorCache.Add(ExecClass, NewExec);
    return NewExec;
}

FSpellExecContextV3 USpellRuntimeV3::BuildExecContext() const
{
    FSpellExecContextV3 Ctx;
    Ctx.WorldContext = WorldContext;
    Ctx.Caster = Caster;
    Ctx.Runtime = const_cast<USpellRuntimeV3*>(this);
    Ctx.EventBus = EventBus;
    Ctx.VariableStore = VariableStore;
    Ctx.TargetStore = TargetStore;
    Ctx.ActionRegistry = Registry;
    Ctx.Seed = Seed;
    Ctx.bAuthority = bAuthority;
    return Ctx;
}

void USpellRuntimeV3::InitializeRng(int32 InSeed)
{
    Seed = InSeed;
    RandomStream.Initialize(Seed);
}


void USpellRuntimeV3::StartFromAsset(USpellSpecV3* InSpecAsset, const FSpellExecContextV3& Ctx)
{
    UObject* WC = Ctx.WorldContext.Get() ? Ctx.WorldContext.Get() : WorldContext.Get();
    AActor*  C  = Ctx.Caster.Get()       ? Ctx.Caster.Get()       : Caster.Get();
    USpellActionRegistryV3* R  = Ctx.ActionRegistry.Get() ? Ctx.ActionRegistry.Get() : Registry.Get();
    USpellEventBusSubsystemV3* EB = Ctx.EventBus.Get() ? Ctx.EventBus.Get() : EventBus.Get();


    if (!WC || !C || !R || !EB || !InSpecAsset)
    {
        UE_LOG(LogTemp, Error, TEXT("StartFromAsset missing context. WC=%s Caster=%s Registry=%s EventBus=%s Spec=%s"),
            *GetNameSafe(WC), *GetNameSafe(C), *GetNameSafe(R), *GetNameSafe(EB), *GetNameSafe(InSpecAsset));
        return;
    }

    // Final: Runtime always has stores
    if (!VariableStore)
    {
        VariableStore = Ctx.VariableStore.Get()
            ? Ctx.VariableStore.Get()
            : NewObject<USpellVariableStoreV3>(this);
    }

    if (!TargetStore)
    {
        TargetStore = Ctx.TargetStore.Get()
            ? Ctx.TargetStore.Get()
            : NewObject<USpellTargetStoreV3>(this);
    }


    // Determinism
    Seed = (Ctx.Seed != 0) ? Ctx.Seed : Seed;
    InitializeRng(Seed);
    bAuthority = Ctx.bAuthority;

    Init(WC, C, InSpecAsset, R, EB);
    Start();
}

void USpellRuntimeV3::BeginDestroy()
{
    Stop();
    Super::BeginDestroy();
}

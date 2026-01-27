#include "Runtime/SpellRuntimeV3.h"

#include "Stores/SpellVariableStoreV3.h"
#include "Stores/SpellTargetStoreV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "VM/SpellVMV3.h"
#include "Spec/SpellSpecV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogIMOPSpellRuntimeV3, Log, All);

static FGameplayTag GetSubscribeFilterTag(const FSpellTriggerMatcherV3& Trigger)
{
	// Subscribe filter is only a coarse pre-filter for the bus.
	// ExactTag is the best cheap filter; otherwise subscribe-all and match via Trigger.MatchesEvent(Ev).
	if (Trigger.bUseExactTag && Trigger.ExactTag.IsValid())
	{
		return Trigger.ExactTag;
	}
	return FGameplayTag(); // empty => receive all
}

void USpellRuntimeV3::Init(
	UObject* WorldContextObj,
	AActor* InCaster,
	USpellSpecV3* InSpec,
	USpellActionRegistryV3* InRegistry,
	USpellEventBusSubsystemV3* InEventBus)
{
	WorldContext = WorldContextObj;
	Caster = InCaster;
	Spec = InSpec;
	Registry = InRegistry;
	EventBus = InEventBus;

	VariableStore = NewObject<USpellVariableStoreV3>(this);
	TargetStore   = NewObject<USpellTargetStoreV3>(this);

	if (!RuntimeGuid.IsValid())
	{
		RuntimeGuid = FGuid::NewGuid();
	}
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

	// Subscribe all handlers (bus supports only FGameplayTag coarse filters)
	for (const FSpellHandlerV3& H : Spec->Handlers)
	{
		const FGameplayTag Filter = GetSubscribeFilterTag(H.Trigger);
		Subscriptions.Add(EventBus->Subscribe(static_cast<ISpellEventListenerV3*>(this), Filter));
	}

	// Subscribe phases too
	for (const FSpellPhaseV3& P : Spec->Phases)
	{
		const FGameplayTag Filter = GetSubscribeFilterTag(P.Trigger);
		Subscriptions.Add(EventBus->Subscribe(static_cast<ISpellEventListenerV3*>(this), Filter));
	}

	// Emit Spell.Start (optional convention)
	FSpellEventV3 Ev;
	Ev.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.Start")));
	Ev.Caster = Caster;
	Ev.Sender = this;
	Ev.Instigator = Caster;
	Ev.RuntimeGuid = RuntimeGuid;

	if (UWorld* W = GetWorld())
	{
		Ev.FrameNumber = (int32)GFrameCounter;
		Ev.TimeSeconds = (float)W->GetTimeSeconds();
	}

	EventBus->Emit(Ev);
	UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Runtime: Start %s"), *GetName());
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
			EventBus->Unsubscribe(H);
		}
	}
	Subscriptions.Reset();

	UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("SpellRuntimeV3: Stopped and unsubscribed."));

	// Emit Spell.End (optional)
	if (EventBus)
	{
		FSpellEventV3 Ev;
		Ev.EventTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Spell.Event.Spell.End")));
		Ev.Caster = Caster;
		Ev.Sender = this;
		Ev.Instigator = Caster;
		Ev.RuntimeGuid = RuntimeGuid;

		if (UWorld* W = GetWorld())
		{
			Ev.FrameNumber = (int32)GFrameCounter;
			Ev.TimeSeconds = (float)W->GetTimeSeconds();
		}

		EventBus->Emit(Ev);
	}

	UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Runtime: Stop %s"), *GetName());
}

void USpellRuntimeV3::OnSpellEvent(const FSpellEventV3& Ev)
{
	if (!bRunning || !Spec) return;

	// Accept only:
	// 1) events sent by THIS runtime (Sender == this)
	// 2) events explicitly targeted to THIS runtime via RuntimeGuid
	const bool bSenderMatches = (Ev.Sender == this);
	const bool bGuidMatches = (Ev.RuntimeGuid.IsValid() && Ev.RuntimeGuid == RuntimeGuid);

	if (!bSenderMatches && !bGuidMatches)
	{
		UE_LOG(LogIMOPSpellRuntimeV3, VeryVerbose, TEXT("OnSpellEvent REJECT: tag=%s sender=%s evGuid=%s runtimeGuid=%s"),
			*Ev.EventTag.ToString(), *GetNameSafe(Ev.Sender.Get()), *Ev.RuntimeGuid.ToString(), *RuntimeGuid.ToString());
		return;
	}

	UE_LOG(LogIMOPSpellRuntimeV3, Verbose, TEXT("OnSpellEvent ACCEPT: tag=%s sender=%s guid=%s"),
		*Ev.EventTag.ToString(), *GetNameSafe(Ev.Sender.Get()), *Ev.RuntimeGuid.ToString());

	// Dispatch handlers that match
	for (const FSpellHandlerV3& H : Spec->Handlers)
	{
		if (!H.Trigger.Matches(Ev.EventTag))
		{
			continue;
		}

		const int32 ActionCount = H.Actions.Num();
		UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Handler TRIGGER: tag=%s actions=%d"), *Ev.EventTag.ToString(), ActionCount);

		if (ActionCount == 0)
		{
			UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Handler DONE (no actions): tag=%s"), *Ev.EventTag.ToString());
			continue;
		}

		if (H.Trigger.MatchesEvent(Ev))
		{
			FSpellVMV3::ExecuteActions(*this, H.Actions);
		}

		UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Handler DONE: tag=%s"), *Ev.EventTag.ToString());
	}

	// Dispatch phases that match
	for (const FSpellPhaseV3& P : Spec->Phases)
	{
		if (!P.Trigger.Matches(Ev.EventTag))
		{
			continue;
		}

		const int32 ActionCount = P.Actions.Num();
		UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Phase TRIGGER: tag=%s actions=%d"), *Ev.EventTag.ToString(), ActionCount);

		if (ActionCount == 0)
		{
			UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Phase DONE (no actions): tag=%s"), *Ev.EventTag.ToString());
			continue;
		}

		if (P.Trigger.MatchesEvent(Ev))
		{
			FSpellVMV3::ExecuteActions(*this, P.Actions);
		}

		UE_LOG(LogIMOPSpellRuntimeV3, Log, TEXT("Phase DONE: tag=%s"), *Ev.EventTag.ToString());
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
	AActor* C = Ctx.Caster.Get() ? Ctx.Caster.Get() : Caster.Get();
	USpellActionRegistryV3* R = Ctx.ActionRegistry.Get() ? Ctx.ActionRegistry.Get() : Registry.Get();
	USpellEventBusSubsystemV3* EB = Ctx.EventBus.Get() ? Ctx.EventBus.Get() : EventBus.Get();

	if (!WC || !C || !R || !EB || !InSpecAsset)
	{
		UE_LOG(LogIMOPSpellRuntimeV3, Error, TEXT("StartFromAsset missing context. WC=%s Caster=%s Registry=%s EventBus=%s Spec=%s"),
			*GetNameSafe(WC), *GetNameSafe(C), *GetNameSafe(R), *GetNameSafe(EB), *GetNameSafe(InSpecAsset));
		return;
	}

	// Final: Runtime always has stores
	if (!VariableStore)
	{
		VariableStore = Ctx.VariableStore.Get() ? Ctx.VariableStore.Get() : NewObject<USpellVariableStoreV3>(this);
	}
	if (!TargetStore)
	{
		TargetStore = Ctx.TargetStore.Get() ? Ctx.TargetStore.Get() : NewObject<USpellTargetStoreV3>(this);
	}

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

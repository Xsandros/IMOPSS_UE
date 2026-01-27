#include "Casting/SpellCastingComponentV3.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Core/SpellCoreSubsystemV3.h"
#include "Actions/SpellActionRegistryV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

USpellCastingComponentV3::USpellCastingComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void USpellCastingComponentV3::BeginPlay()
{
	Super::BeginPlay();
}

bool USpellCastingComponentV3::CastLoadoutIndex(int32 Index)
{
	if (!Loadout.IsValidIndex(Index))
	{
		UE_LOG(LogTemp, Warning, TEXT("CastLoadoutIndex: invalid index %d"), Index);
		return false;
	}

	return CastSpell(Loadout[Index]);
}

bool USpellCastingComponentV3::CastSpell(USpellSpecV3* Spec)
{
	if (!Spec)
	{
		UE_LOG(LogTemp, Warning, TEXT("CastSpell: Spec is null"));
		return false;
	}

	// If we're a client, route to server.
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	if (!Owner->HasAuthority())
	{
		ServerCastSpell(Spec);
		return true;
	}

	// Server executes immediately.
	CastSpell_Internal(Spec);
	return true;
}

void USpellCastingComponentV3::ServerCastSpell_Implementation(USpellSpecV3* Spec)
{
	CastSpell_Internal(Spec);
}

void USpellCastingComponentV3::CastSpell_Internal(USpellSpecV3* Spec)
{
	if (!Spec) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UGameInstance* GI = W->GetGameInstance();
	if (!GI) return;

	USpellCoreSubsystemV3* Core = GI->GetSubsystem<USpellCoreSubsystemV3>();
	USpellEventBusSubsystemV3* EventBus = W ? W->GetSubsystem<USpellEventBusSubsystemV3>() : nullptr;

	if (!Core || !EventBus)
	{
		UE_LOG(LogTemp, Error, TEXT("CastSpell_Internal: missing Core/EventBus subsystem."));
		return;
	}

	USpellActionRegistryV3* Registry = Core->GetActionRegistry();
	if (!Registry)
	{
		UE_LOG(LogTemp, Error, TEXT("CastSpell_Internal: missing ActionRegistry."));
		return;
	}

	// Create a runtime owned by this component (easy lifetime management).
	USpellRuntimeV3* Runtime = NewObject<USpellRuntimeV3>(this);
	if (!Runtime)
	{
		UE_LOG(LogTemp, Error, TEXT("CastSpell_Internal: failed to allocate runtime."));
		return;
	}

	// Deterministic seed on server; clients just request.
	// CastCounter makes repeated casts differ while still being deterministic across server replays (given same order).
	const int32 Seed = BaseSeed + (++CastCounter);

	Runtime->Init(this, Owner, Spec, Registry, EventBus); // uses WorldContextObj (this) + caster (owner) :contentReference[oaicite:3]{index=3}
	Runtime->InitializeRng(Seed);
	Runtime->Start();

	LastRuntime = Runtime;

	if (bKeepRuntimesAliveForDebug)
	{
		ActiveRuntimes.Add(Runtime);
	}
}

void USpellCastingComponentV3::StopAll()
{
	// Only the authority should stop server-owned runtimes (clients don't own state).
	AActor* Owner = GetOwner();
	if (Owner && !Owner->HasAuthority())
	{
		// Optional: you could add a ServerStopAll RPC later if you want client buttons for it.
		UE_LOG(LogTemp, Warning, TEXT("StopAll called on client; ignoring."));
		return;
	}

	for (USpellRuntimeV3* R : ActiveRuntimes)
	{
		if (R)
		{
			R->Stop();
		}
	}
	ActiveRuntimes.Reset();
	LastRuntime = nullptr;
}

FGuid USpellCastingComponentV3::GetLastRuntimeGuid() const
{
	if (LastRuntime)
	{
		return LastRuntime->GetRuntimeGuid();
	}
	return FGuid();
}

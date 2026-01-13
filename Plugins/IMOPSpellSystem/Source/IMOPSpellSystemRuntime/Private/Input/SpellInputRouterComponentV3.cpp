#include "Input/SpellInputRouterComponentV3.h"

#include "Input/SpellInputBindingsAssetV3.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

#include "Runtime/SpellRuntimeV3.h"
#include "Core/SpellCoreSubsystemV3.h"
#include "Events/SpellEventBusSubsystemV3.h"
#include "Actions/SpellActionRegistryV3.h"

USpellInputRouterComponentV3::USpellInputRouterComponentV3()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void USpellInputRouterComponentV3::BeginPlay()
{
	Super::BeginPlay();

	if (!BindingsAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpellInputRouter: No BindingsAsset set."));
		return;
	}

	// Build Action->Index map (idempotent)
	ActionToIndex.Reset();
	for (int32 i = 0; i < BindingsAsset->Bindings.Num(); ++i)
	{
		const auto& Slot = BindingsAsset->Bindings[i];
		if (Slot.Action)
		{
			ActionToIndex.Add(Slot.Action, i);
		}
	}

	// Mapping context is OK in BeginPlay, but must be idempotent
	AddMappingContextIfPossible();

	// IMPORTANT: Bindings are NOT done here anymore (prevents stacking)
	BindInputIfPossible();
}

void USpellInputRouterComponentV3::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInputIfNeeded();
	RemoveMappingContextIfPossible();

	Super::EndPlay(EndPlayReason);
}

UEnhancedInputComponent* USpellInputRouterComponentV3::GetEnhancedInputComponent() const
{
	// Prefer owner's InputComponent if owner is a Controller
	if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		return Cast<UEnhancedInputComponent>(PC->InputComponent);
	}

	// Otherwise: if owner is a Pawn, try its controller
	if (const APawn* P = Cast<APawn>(GetOwner()))
	{
		if (const APlayerController* PC = Cast<APlayerController>(P->GetController()))
		{
			return Cast<UEnhancedInputComponent>(PC->InputComponent);
		}
	}

	return nullptr;
}

void USpellInputRouterComponentV3::BindInputIfPossible()
{
	if (bInputBound)
	{
		return; // idempotent
	}
	if (!BindingsAsset)
	{
		return;
	}

	UEnhancedInputComponent* EIC = GetEnhancedInputComponent();
	if (!EIC)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpellInputRouter: No EnhancedInputComponent found."));
		return;
	}

	BindingHandles.Reset();
	BindingHandles.Reserve(BindingsAsset->Bindings.Num());

	for (int32 i = 0; i < BindingsAsset->Bindings.Num(); ++i)
	{
		const auto& Slot = BindingsAsset->Bindings[i];
		if (!Slot.Action) continue;

		// Store binding handle so we can remove it later.
		FEnhancedInputActionEventBinding& B = EIC->BindAction(
			Slot.Action,
			Slot.TriggerEvent,
			this,
			&USpellInputRouterComponentV3::OnAnyInputAction
		);

		BindingHandles.Add(B.GetHandle());
	}

	bInputBound = true;

	UE_LOG(LogTemp, Log, TEXT("SpellInputRouter: Bound %d actions."), BindingHandles.Num());
}

void USpellInputRouterComponentV3::UnbindInputIfNeeded()
{
	if (!bInputBound)
	{
		return;
	}

	UEnhancedInputComponent* EIC = GetEnhancedInputComponent();
	if (EIC)
	{
		for (int32 Handle : BindingHandles)
		{
			EIC->RemoveBindingByHandle(Handle);
		}
	}

	BindingHandles.Reset();
	bInputBound = false;

	UE_LOG(LogTemp, Log, TEXT("SpellInputRouter: Unbound actions."));
}

void USpellInputRouterComponentV3::AddMappingContextIfPossible()
{
	if (bAddedContext) return;
	if (!BindingsAsset || !BindingsAsset->MappingContext) return;

	APlayerController* PC = nullptr;
	if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		PC = OwnerPC;
	}
	else if (APawn* P = Cast<APawn>(GetOwner()))
	{
		PC = Cast<APlayerController>(P->GetController());
	}

	if (!PC) return;

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsys->AddMappingContext(const_cast<UInputMappingContext*>(BindingsAsset->MappingContext.Get()), BindingsAsset->Priority);
		bAddedContext = true;
	}
}

void USpellInputRouterComponentV3::RemoveMappingContextIfPossible()
{
	if (!bAddedContext) return;
	if (!BindingsAsset || !BindingsAsset->MappingContext) return;

	APlayerController* PC = nullptr;
	if (APlayerController* OwnerPC = Cast<APlayerController>(GetOwner()))
	{
		PC = OwnerPC;
	}
	else if (APawn* P = Cast<APawn>(GetOwner()))
	{
		PC = Cast<APlayerController>(P->GetController());
	}

	if (!PC) return;

	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;

	if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
	{
		Subsys->RemoveMappingContext(const_cast<UInputMappingContext*>(BindingsAsset->MappingContext.Get()));
	}

	bAddedContext = false;
}

void USpellInputRouterComponentV3::OnAnyInputAction(const FInputActionInstance& Instance)
{
	const UInputAction* SourceAction = Instance.GetSourceAction();
	if (!SourceAction) return;

	const int32* SlotIndexPtr = ActionToIndex.Find(SourceAction);
	if (!SlotIndexPtr) return;

	ExecuteSlotByIndex(*SlotIndexPtr);
}

void USpellInputRouterComponentV3::ExecuteSlotByIndex(int32 SlotIndex)
{
	if (!BindingsAsset) return;
	if (!BindingsAsset->Bindings.IsValidIndex(SlotIndex)) return;

	UE_LOG(LogTemp, Log, TEXT("Router: Execute slot %d"), SlotIndex);

	
	// ---- Stop previous runtime for this slot (prevents stacking)
	if (TObjectPtr<USpellRuntimeV3>* ExistingPtr = ActiveRuntimeBySlot.Find(SlotIndex))
	{
		if (USpellRuntimeV3* Existing = ExistingPtr->Get())
		{
			UE_LOG(LogTemp, Log, TEXT("SpellInputRouter: Stopping previous runtime for slot %d"), SlotIndex);
			Existing->Stop();
		}
		ActiveRuntimeBySlot.Remove(SlotIndex);
	}

	
	const FSpellInputBindingSlotV3& Slot = BindingsAsset->Bindings[SlotIndex];
	if (!Slot.Spell)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpellInputRouter: Slot %d has no Spell."), SlotIndex);
		return;
	}

	// Caster: FINAL-FORM for PC owner -> use pawn; for pawn owner -> use pawn
	AActor* Caster = nullptr;

	if (APlayerController* PC = Cast<APlayerController>(GetOwner()))
	{
		Caster = PC->GetPawn();
	}
	else
	{
		Caster = Cast<AActor>(GetOwner());
	}

	if (!Caster) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	USpellCoreSubsystemV3* Core = GI->GetSubsystem<USpellCoreSubsystemV3>();
	if (!Core)
	{
		UE_LOG(LogTemp, Error, TEXT("SpellInputRouter: SpellCoreSubsystemV3 missing."));
		return;
	}

	USpellActionRegistryV3* Registry = Core->GetActionRegistry();
	USpellEventBusSubsystemV3* EventBus = GI->GetSubsystem<USpellEventBusSubsystemV3>();

	if (!Registry || !EventBus)
	{
		UE_LOG(LogTemp, Error, TEXT("SpellInputRouter: Registry or EventBus missing."));
		return;
	}

	USpellRuntimeV3* Runtime = NewObject<USpellRuntimeV3>(GetOwner());
	Runtime->Init(World, Caster, Slot.Spell, Registry, EventBus);

	const int32 SeedToUse = Slot.bOverrideSeed ? Slot.SeedOverride : 1337;
	Runtime->InitializeRng(SeedToUse);

	Runtime->Start();
	ActiveRuntimeBySlot.Add(SlotIndex, Runtime);

}
